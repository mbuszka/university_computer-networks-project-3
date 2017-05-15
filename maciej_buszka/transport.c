/*
  Maciej Buszka
  279129
*/

#include <stdio.h>
#include <stdlib.h>
#include "udp.h"
#include "util.h"

void show_usage(char *prog) {
  printf("Usage:\n%s PORT FILE SIZE\n"
         "PORT - short integer\n"
         "FILE - filename without spaces\n"
         "SIZE - size in bytes\n"
         , prog);
}

void check_arguments(uint16_t *port,
                     char *filename,
                     size_t *file_size,
                     int argc,
                     char *argv[]) {
  if (argc != 4) {
    show_usage(argv[0]);
    printf("All arguments are mandatory\n");
    exit(0);
  }
  if (0 == sscanf(argv[1], "%hd", port)) {
    printf("PORT argument was invalid\n\n");
    show_usage(argv[0]);
    exit(0);
  }
  if (0 == sscanf(argv[2], "%s", filename)) {
    show_usage(argv[0]);
    exit(0);
  }
  if (0 == sscanf(argv[3], "%lu", file_size)) {
    printf("SIZE argument was invalid\n\n");
    show_usage(argv[0]);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  uint16_t port;
  char     filename[256];
  size_t   file_size;
  
  check_arguments(&port, filename, &file_size, argc, argv);

  size_t   chunk_size = BUFFER_SIZE;
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
