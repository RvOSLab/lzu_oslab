

- [ ] 中断
- [ ] 地址映射

# 目的

提供 *boot program* 和 *client program* 之间统一的借口

# 格式

DTB 和 DTS

# 使用方法

boot program 探测设备，并将设备表示为设备树，存储到内存中并将指向该内存的指针传递给 client program



# 结构

树装结构，每个节点代表一个设备

# 命名

`node-name@unit-address`

> The unit-address must match the ﬁrst address speciﬁed in the
> reg property of the node. If the node has no reg property, the @unit-address must be omitted and the node-name alone
> differentiates the node from other nodes at the same level in the tree.

根节点命名为`/`，其他节点根据设备类型，规范推荐了一些名字

## 定位

可以使用 UNIX 风格路径进行定位：`/node-name-1/node-name-2/node-name-N`

*phandle* 属性似乎也是一种方法，相当于节点 ID？

# 属性

每个节点中有多个属性-值键值对，标准允许厂商提供非标准的属性，非标准属性需要加上独一无二的前缀来表示该属性是某厂商添加的，如`linux,network-index`。

## 属性值

值有一下几种类型：------------------

注意，`<u32`和`u64`都是大端表示。



