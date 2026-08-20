#include "capture_buffer.h"

CaptureBuffer::CaptureBuffer() : head(0), count(0)
{
}

void CaptureBuffer::clear()
{
    head = 0;
    count = 0;
}

void CaptureBuffer::push(const Sample& sample)
{
    buffer[head] = sample;
    head = (head + 1) % CAPTURE_BUFFER_SIZE;

    if (count < CAPTURE_BUFFER_SIZE)
    {
        count++;
    }
}

bool CaptureBuffer::full() const
{
    return count == CAPTURE_BUFFER_SIZE;
}

uint16_t CaptureBuffer::size() const
{
    return count;
}

uint16_t CaptureBuffer::capacity() const
{
    return CAPTURE_BUFFER_SIZE;
}

bool CaptureBuffer::get(uint16_t index, Sample& out) const
{
    if (index >= count)
    {
        return false;
    }

    const uint16_t oldest = (head + CAPTURE_BUFFER_SIZE - count) % CAPTURE_BUFFER_SIZE;
    const uint16_t actual = (oldest + index) % CAPTURE_BUFFER_SIZE;
    out = buffer[actual];
    return true;
}

int16_t CaptureBuffer::findFirstTickAtOrAfter(uint16_t tick) const
{
    Sample sample;
    for (uint16_t i = 0; i < count; i++)
    {
        if (!get(i, sample))
        {
            return -1;
        }

        if (sample.tick >= tick)
        {
            return static_cast<int16_t>(i);
        }
    }

    return -1;
}
