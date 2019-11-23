# 简单的正则表达式的C语言实现

[参考网站]( https://swtch.com/~rsc/regexp/regexp1.html )

## 转换为后缀形式

首先需要将输入的正则表达式转换为后缀形式，例如：

> "a(bb)+a"转换为"abb.+.a"。

**转换规则：**用''**.**''来表示连接。

## 转换为NFA

使用多个**State**结构体的链接表示NFA：

```C
/*
 * Represents an NFA state plus zero or one or two arrows exiting.
 * if c == 256, no arrows out; matching state.
 * If c == 257, unlabeled arrows to out and out1 (if != NULL).
 * If c < 256, labeled arrow with character c to out.
 */
struct State
{
	int c;
	State *out;
	State *out1;
	int lastlist;
};
```

> 一个State最多只有两个分支。每个State都代表着一个NFA的片段。

对NFA片段的描述采用Frag结构体：

```C
struct Frag
{
	State *start;
	Ptrlist *out;
};
```

> start表示开始状态；out表示这个NFA片段中未连接到其他State的悬挂指针，是一系列的**State***的指针



```C
union Ptrlist
{
	Ptrlist *next;
	State *s;
};
```

