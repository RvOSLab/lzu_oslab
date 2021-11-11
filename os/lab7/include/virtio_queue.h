#ifndef VIRTQUEUE_H
#define VIRTQUEUE_H

#include <stddef.h>
#include <mm.h>

#define VIRTQ_RING_NUM 16
#define VIRTQ_DESC_TABLE_LENGTH (16 * VIRTQ_RING_NUM)
#define VIRTQ_AVAIL_RING_LENGTH (6 + 2 * VIRTQ_RING_NUM)
#define VIRTQ_USED_RING_LENGTH  (6 + 8 * VIRTQ_RING_NUM)
#define VIRTQ_LENGTH (CEIL( \
    VIRTQ_DESC_TABLE_LENGTH + \
    VIRTQ_AVAIL_RING_LENGTH + \
    VIRTQ_USED_RING_LENGTH \
))

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VIRTQ_DESC_F_WRITE      2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT   4

/* The device uses this in used->flags to advise the driver: don't kick me
 * when you add a buffer.  It's unreliable, so it's simply an
 * optimization. */
#define VIRTQ_USED_F_NO_NOTIFY  1
/* The driver uses this in avail->flags to advise the device: don't
 * interrupt me when you consume a buffer.  It's unreliable, so it's
 * simply an optimization.  */
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1

/* Support for indirect descriptors */
#define VIRTIO_F_INDIRECT_DESC    28

/* Support for avail_event and used_event fields */
#define VIRTIO_F_EVENT_IDX        29

/* Arbitrary descriptor layouts. */
#define VIRTIO_F_ANY_LAYOUT       27

/* Virtqueue descriptors: 16 bytes.
 * These can chain together via "next". */
struct virtq_desc {
        /* Address (guest-physical). */
        uint64_t addr;
        /* Length. */
        uint32_t len;
        /* The flags as indicated above. */
        uint16_t flags;
        /* We chain unused descriptors via this, too */
        uint16_t next;
};

struct virtq_avail {
        uint16_t flags;
        uint16_t idx;
        uint16_t ring[];
        /* Only if VIRTIO_F_EVENT_IDX: uint16_t used_event; */
};

/* uint32_t is used here for ids for padding reasons. */
struct virtq_used_elem {
        /* Index of start of used descriptor chain. */
        uint32_t id;
        /* Total length of the descriptor chain which was written to. */
        uint32_t len;
};

struct virtq_used {
        uint16_t flags;
        uint16_t idx;
        struct virtq_used_elem ring[];
        /* Only if VIRTIO_F_EVENT_IDX: uint16_t avail_event; */
};

struct virtq {
        unsigned int num;

        struct virtq_desc *desc;
        struct virtq_avail *avail;
        struct virtq_used *used;
};

static inline int virtq_need_event(uint16_t event_idx, uint16_t new_idx, uint16_t old_idx)
{
         return (uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old_idx);
}

/* Get location of event indices (only with VIRTIO_F_EVENT_IDX) */
static inline uint16_t *virtq_used_event(struct virtq *vq)
{
        /* For backwards compat, used event index is at *end* of avail ring. */
        return &vq->avail->ring[vq->num];
}

static inline uint16_t *virtq_avail_event(struct virtq *vq)
{
        /* For backwards compat, avail event index is at *end* of used ring. */
        return (uint16_t *)&vq->used->ring[vq->num];
}
#endif /* VIRTQUEUE_H */
