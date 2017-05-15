/*
  Maciej Buszka
  279129
*/

#include <stdint.h>

#define BUFFER_SIZE 1000
#define WINDOW_LEN 500

typedef enum {
  STATUS_SENT,
  STATUS_UNUSED,
  STATUS_RECEIVED,
} status_t;

typedef struct {
  status_t status;
  size_t   begin;
  size_t   size;
  ssize_t  timeout;
  char     data[BUFFER_SIZE];
} segment_t;

void init_connection(uint16_t port);
int send_request(size_t begin, size_t size);
int receive_segment();
void store_segments(FILE *file);
int is_finished();
void handle_segments(FILE *file);
void await_segments();
