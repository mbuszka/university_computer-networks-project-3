/*
  Maciej Buszka
  279129
*/

#include <stdio.h>
#include <stdlib.h>
#include "udp.h"
#include "util.h"

int main(int argc, char *argv[]) {
  uint16_t port;
  char     filename[256];
  size_t   file_size;
  sscanf(argv[1], "%hd", &port);
  sscanf(argv[2], "%s", filename);
  sscanf(argv[3], "%lu", &file_size);
  size_t   chunk_size = min(BUFFER_SIZE, file_size / WINDOW_LEN);
  chunk_size = max(chunk_size, BUFFER_SIZE);
  size_t   full_packets = file_size / chunk_size;
  size_t   last_packet  = file_size % chunk_size;

  init_connection(port);
  FILE *file = fopen(filename, "w");

  for (size_t i=0; i<full_packets; i++) {
    while (send_request(i * chunk_size, chunk_size) < 0) {
      await_segments();
      handle_segments(file);
    }
  }
  if (last_packet > 0) {
    while (send_request(full_packets * chunk_size, last_packet) < 0) {
      await_segments();
      handle_segments(file);
    }
  }
  while (!is_finished()) {
    await_segments();
    handle_segments(file);
  }

  fclose(file);
  return 0;
}
