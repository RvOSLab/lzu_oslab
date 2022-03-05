#include <device/virtio/virtio_queue.h>

#include <mm.h>
#include <string.h>

uint16_t virtq_get_desc(struct virtq *vq) {
    uint16_t next_idx = vq->next_empty_desc;
    if(next_idx != 0xff) {
        vq->next_empty_desc = vq->desc[next_idx].next;
    }
    return next_idx;
}

void virtq_free_desc(struct virtq *vq, uint16_t idx) {
    vq->desc[idx].next = vq->next_empty_desc;
    vq->next_empty_desc = idx;
}

void virtq_free_desc_chain(struct virtq *vq, uint16_t idx) {
    uint16_t tmp_idx;
    while (vq->desc[idx].flags & VIRTQ_DESC_F_NEXT)
    {
        tmp_idx = vq->desc[idx].next;
        virtq_free_desc(vq, idx);
        idx = tmp_idx;
    }
    virtq_free_desc(vq, idx);
}

void virtq_put_avail(struct virtq *vq, uint16_t idx) {
    vq->avail->ring[vq->avail->idx % VIRTQ_RING_NUM] = idx;
    synchronize();
    vq->avail->idx += 1;
    synchronize();
}

struct virtq_used_elem* virtq_get_used_elem(struct virtq *vq) {
    if(vq->used->idx == vq->last_used_idx) {
        return NULL;
    } else {
        struct virtq_used_elem* used_elem = vq->used->ring + (vq->last_used_idx % VIRTQ_RING_NUM);
        vq->last_used_idx += 1;
        return used_elem;
    }
}

void virtio_queue_init(struct virtq* virtio_queue, uint64_t is_legacy) {
    uint64_t virtq_legacy_mem = get_free_page();
    uint64_t virtq_phy_addr = get_free_page();
    uint64_t virtq_vir_addr = VIRTUAL(virtq_phy_addr);
    memset((uint8_t *)virtq_vir_addr, 0, VIRTQ_LENGTH);
    virtio_queue->num = VIRTQ_RING_NUM;
    virtio_queue->desc  = (struct virtq_desc *)  (virtq_vir_addr);
    virtio_queue->avail = (struct virtq_avail *) (virtq_vir_addr + VIRTQ_DESC_TABLE_LENGTH);
    if (is_legacy) {
        /* Each virtqueue occupies two or more physically-contiguous pages */
        virtio_queue->used  = (struct virtq_used *) VIRTUAL(virtq_legacy_mem);
    } else {
        free_page(virtq_legacy_mem);
        virtio_queue->used  = (struct virtq_used *) (virtq_vir_addr + VIRTQ_DESC_TABLE_LENGTH + VIRTQ_AVAIL_RING_LENGTH);
    }
    virtio_queue->last_used_idx = 0;
    virtio_queue->physical_addr = virtq_phy_addr;
    uint16_t idx;
    for (idx = 0; idx < VIRTQ_RING_NUM - 1; ++idx) {
        virtio_queue->desc[idx].next = idx + 1;
    }
    virtio_queue->desc[idx].next = 0xff;
}
