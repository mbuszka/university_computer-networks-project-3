/*
  Maciej Buszka
  279129
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "udp.h"
#include "util.h"

#define DEBUG 0
#define TRACE 0
#define PORT 12345
#define SERVER_ADDR "156.17.4.30"
#define DEFAULT_TIMEOUT 100
#define trace_fun() do { if (TRACE) printf("%s\n", __func__); } while(0)

static segment_t window[WINDOW_LEN];
static size_t next_segment  = 0;
static size_t first_segment = 0;
static uint16_t server_port = 0;
static int socket_fd;

void init_connection(uint16_t port) {
  trace_fun();
  server_port = port;

  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) fail(errno);

  struct sockaddr_in server_address;
  bzero (&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  if (0 > bind(socket_fd,
               (struct sockaddr*) &server_address,
               sizeof(server_address))) fail(errno);

  for (int i=0; i<WINDOW_LEN; i++) {
    window[i].status = STATUS_UNUSED;
  }
}

void send_segment(int idx) {
  trace_fun();
  char buffer[BUFFER_SIZE];
  sprintf(buffer, "GET %lu %lu\n", window[idx].begin, window[idx].size);

  struct sockaddr_in recipient;
  bzero (&recipient, sizeof(recipient));

  recipient.sin_family = AF_INET;
  recipient.sin_port = htons(server_port);
  inet_pton(AF_INET, SERVER_ADDR, &recipient.sin_addr);

  sendto( socket_fd, buffer, strlen(buffer), 0
        , (struct sockaddr*) &recipient, sizeof(recipient)
        );

  window[idx].timeout = DEFAULT_TIMEOUT;
  window[idx].status  = STATUS_SENT;
}

int send_request(size_t begin, size_t size) {
  trace_fun();
  if (next_segment == first_segment &&
      window[first_segment].status != STATUS_UNUSED) return -1;

  window[next_segment].status  = STATUS_SENT;
  window[next_segment].begin   = begin;
  window[next_segment].size    = size;
  window[next_segment].timeout = DEFAULT_TIMEOUT;

  send_segment(next_segment);
  next_segment++;
  next_segment %= WINDOW_LEN;
  return 0;
}

int find_segment(size_t begin) {
  trace_fun();
  for (int i=0; i<WINDOW_LEN; i++) {
    if (window[i].begin == begin) return i;
  }

  return -1;
}

int receive_segment() {
  trace_fun();
  size_t             begin;
  size_t             size;
  int                idx;
  char              *data_start;
  char               ip_str[40];
  struct sockaddr_in sender;
  socklen_t          sender_len = sizeof(sender);
  char               buffer[IP_MAXPACKET+1];

  ssize_t packet_len = recvfrom(
    socket_fd,
    buffer,
    IP_MAXPACKET,
    MSG_DONTWAIT,
    (struct sockaddr*) &sender,
    &sender_len
  );

  if (packet_len < 0) {
    return -errno;
  }

  inet_ntop(AF_INET, &sender.sin_addr, ip_str, sizeof(ip_str));
  if (strcmp(SERVER_ADDR, ip_str) != 0) {
    if (DEBUG) printf("wrong ip %s\n", ip_str);
    return 0;
  }
  if (ntohs(sender.sin_port) != server_port) {
    if (DEBUG) printf("wrong port\n");
    return 0;
  }


  sscanf(buffer, "DATA %lu %lu", &begin, &size);

  idx = find_segment(begin);
  if (window[idx].status != STATUS_SENT) return 0;
  if (idx < 0) return 0;
  data_start = strchr(buffer, '\n') + 1;
  memcpy(window[idx].data, data_start, size);
  window[idx].status = STATUS_RECEIVED;
  return 0;
}

void store_segments(FILE *file) {
  trace_fun();
  while (window[first_segment].status == STATUS_RECEIVED) {
    fwrite(window[first_segment].data,
           sizeof(char),
           window[first_segment].size,
           file);

    window[first_segment].status = STATUS_UNUSED;
    first_segment++;
    first_segment %= WINDOW_LEN;
  }
}

int is_finished() {
  trace_fun();
  for (int i=0; i<WINDOW_LEN; i++) {
    if (window[i].status != STATUS_UNUSED) {
      if (DEBUG) printf("Still waiting for b: %lu, s: %lu\n", window[i].begin, window[i].size);
      return 0;
    }
  }
  return 1;
}

void age_timeouts(size_t msecs) {
  trace_fun();
  for (int i=0; i<WINDOW_LEN; i++) {
    if (window[i].status == STATUS_SENT) window[i].timeout -= msecs;
  }
}

void resend_segments() {
  trace_fun();
  for (int i=0; i<WINDOW_LEN; i++) {
    if (window[i].status == STATUS_SENT && window[i].timeout < 0) {
      send_segment(i);
      send_segment(i);
      send_segment(i);
    }
  }
}

void await_segments() {
  trace_fun();
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  fd_set descriptors;
  FD_ZERO(&descriptors);
  FD_SET(socket_fd, &descriptors);
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 25000;
  select(socket_fd + 1, &descriptors, NULL, NULL, &tv);
  clock_gettime(CLOCK_MONOTONIC, &end);
  age_timeouts((end.tv_sec - start.tv_sec) * 1000 +
               (end.tv_nsec - start.tv_nsec) / 1000000);
  resend_segments();
}

void handle_segments(FILE *file) {
  trace_fun();
  int status;
  while ((status = receive_segment()) != -EWOULDBLOCK) {
    if (status < 0) fail(-status);
  };
  store_segments(file);
}
