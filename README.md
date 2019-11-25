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



## 多状态匹配NFA

根据输入的字符串一步一步的在NFA上前行，用一个状态集合记录这当前所到的状态，看最终能不能走到唯一的匹配状态。



## 将NFA缓存为DFA

前面Thompson的NFA模拟执行其实就是执行了一个等价的DFA，每一个状态的集合就相当于DFA当中的一个结点，DFA的每一步都是确定的。

使用**DState结构体**来描述DFA的状态结点：

```C
struct DState
{
	List l;
	DState *next[256];
	DState *left;
	DState *right;
};
```

一个**DState**是一个集合State的缓存，在这里是**List**结构体。**next**包含了下一个状态的指针，对每一个输入的字符都有一个指针：比如当前状态是d，下一个输入的字符是c，那么d->next[c]就是下一个状态。如果    d->next[c]是空，那么状态还没有被计算出来，需在nfa上前进一步然后缓存下来。



## DFA结点的缓存

在内存有限的环境里，只能分配少量的DState结点，这样在结点缓存的结点不够时就得释放之前的结点，并把这些内存用作新的结点。

> 详情参考dfa_cache.c中的代码

具体实现：

> freelist用来链接空闲的结点，需要时从空闲链表上面拿。
>
> 第一次用的时候需要直接分配内存，当达到了设置了内存上限时释放内存，挂在freelist上，之后都是从freelist上获取结点了。