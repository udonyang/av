#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <memory>
#include <functional>

extern "C" 
{
#include "libavformat/avio.h"
#include "libavformat/avformat.h"
}

using namespace std;

namespace util {
typedef std::map<int, std::vector<std::string>> OptionMap;
int ParseOption(int argc, char** argv, const char* optstring, OptionMap* optmap)
{
  for (;;)
  {
    int opt = getopt(argc, argv, optstring);
    if (opt == -1) 
    {
      break;
    }

    if (opt == '?')
    {
      continue;
    }

    (*optmap)[opt].push_back(optarg);
  }
  return 0;
}

int ReadFile(const std::string& filename, std::string* output)
{
  int ret = 0;

  FILE* input = fopen(filename.c_str(), "r");
  if (input == nullptr)
  {
    cerr << "ERR:" << errno << ": fopen failed: errmsg[" << strerror(errno) << "]" 
        << " filename[" << filename << "]" << endl;
    return -1;
  }

  char buf[BUFSIZ] = "";
  for (;;)
  {
    ret = fread(buf, 1, BUFSIZ, input);
    if (ret > 0)
    {
      output->append(buf, ret);
    }
    else
    {
      if (feof(input))
      {
        break;
      }
      else
      {
        cerr << "ERR:" << ferror(input) << ": fread failed" << endl;
        return -1;
      }
    }
  }

  fclose(input);
  return 0;
}
}

namespace io {
namespace mem {
typedef std::pair<std::string*, int64_t> Opaque;

int Read(void *opaque, uint8_t *buf, int bufsize)
{
  Opaque* buffer = (Opaque*)opaque;
  int end  = buffer->second+bufsize;
  if (end > buffer->first->size()) end = buffer->first->size();
  int size = end-buffer->second;
  if (size == 0) return 0;
  memcpy(buf, buffer->first->data()+buffer->second, size);
  buffer->second += size;
  return size;
}

int Write(void *opaque, uint8_t *buf, int bufsize)
{
  Opaque* buffer = (Opaque*)opaque;
  buffer->first->append((char*)buf, bufsize);
  return bufsize;
}

int64_t Seek(void *opaque, int64_t offset, int whence)
{
  Opaque* buffer = (Opaque*)opaque;
  int64_t pos = -1;
  if (whence == SEEK_CUR)
  {
    pos = buffer->second+offset;
  }
  else if (whence == SEEK_END)
  {
    pos = (int64_t)buffer->first->size()+offset;
  }
  else if (whence == SEEK_SET)
  {
    pos = offset;
  }
  else
  {
    return -2;
  }
  if (pos < 0 || pos >= buffer->first->size()) pos = -2;
  buffer->second = pos;
  return -1;
}
}
}

int Usage(int argc, char** argv)
{
  cerr << "Usage: " << argv[0] << " <command>\n"
      << "<command>\n"
      << "  info -i <inputfile>\n"
      << endl;
  return 0;
}

static int Info(int argc, char** argv)
{
  int ret = 0;
  util::OptionMap optmap;
  ret = util::ParseOption(argc, argv, "i:", &optmap);
  if (ret != 0)
  {
    cerr << "ERR: ParseOption failed"<< endl;
    return ret;
  }

  if (optmap.count('i') == 0)
  {
    return -2;
  }

  av_register_all();
  av_log_set_level(AV_LOG_TRACE);

  std::string input;
  ret = util::ReadFile(optmap['i'][0], &input);
  if (ret != 0)
  {
    cerr << "ERR:" << ret << ": ReadFile failed:"
        << " file[" << optmap['i'][0] << "]"
        << endl;
    return ret;
  }

  AVFormatContext* formatctx = avformat_alloc_context();
  if (formatctx == nullptr) 
  {
    cerr << "ERR: avformat_alloc_context failed" << endl;
    return -1;
  }

  io::mem::Opaque opaque(&input, 0);
  formatctx->pb = avio_alloc_context((uint8_t*)av_malloc(BUFSIZ), BUFSIZ,
                                          0, (void*)&opaque, 
                                          io::mem::Read, nullptr, io::mem::Seek);
  if (formatctx->pb == nullptr)
  {
    cerr << "ERR: avio_alloc_context failed" << endl;
    return -1;
  }

  formatctx->flags |= AVFMT_FLAG_CUSTOM_IO;
  ret = avformat_open_input(&formatctx, nullptr, nullptr, nullptr);
  if (ret != 0)
  {
    cerr << "ERR:" << ret << ": avformat_open_input failed: averr[" << av_err2str(ret) << "]" << endl;
    return -1;
  }

  for (int i = 0; i< formatctx->nb_streams; i++)
  {
    av_dump_format(formatctx, i, nullptr, 0);
  }

  av_freep(&formatctx->pb->buffer);
  av_freep(&formatctx->pb);
  avformat_close_input(&formatctx);
  return 0;
}

static int DumpPacket(int argc, char** argv)
{
  return 0;
}

int main(int argc, char** argv)
{
  if (argc < 2) 
  {
    return Usage(argc, argv);
  }

  std::function<int(int, char**)> command = Usage;
  char* cmdstr = argv[1];
  if (!strcmp(cmdstr, "info"))
  {
    command = Info;
  }
  else if (!strcmp(cmdstr, "dumppacket"))
  {
    command = DumpPacket;
  }

  int ret = command(argc-1, argv+1);
  if (ret == -2)
  {
    return Usage(argc, argv);
  }
  return ret;
}
