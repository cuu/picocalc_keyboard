#include "fifo.h"

static struct {
  struct fifo_item fifo[FIFO_SIZE];
  uint8_t count;
  uint8_t read_idx;
  uint8_t write_idx;
} fifo_self;

uint8_t fifo_count(void)
{
  return fifo_self.count;
}

void fifo_flush(void)
{
  fifo_self.write_idx = 0;
  fifo_self.read_idx = 0;
  fifo_self.count = 0;
}

bool fifo_enqueue(const struct fifo_item item)
{
  if (fifo_self.count >= FIFO_SIZE)
    return false;

  fifo_self.fifo[fifo_self.write_idx++] = item;

  fifo_self.write_idx %= FIFO_SIZE;
  ++fifo_self.count;

  return true;
}

void fifo_enqueue_force(const struct fifo_item item)
{
  if (fifo_enqueue(item))
    return;

  fifo_self.fifo[fifo_self.write_idx++] = item;
  fifo_self.write_idx %= FIFO_SIZE;

  fifo_self.read_idx++;
  fifo_self.read_idx %= FIFO_SIZE;
}

struct fifo_item fifo_dequeue(void)
{
  struct fifo_item item = { 0 };
  if (fifo_self.count == 0)
    return item;

  item = fifo_self.fifo[fifo_self.read_idx++];
  fifo_self.read_idx %= FIFO_SIZE;
  --fifo_self.count;

  return item;
}
