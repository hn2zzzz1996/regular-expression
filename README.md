# 简单的正则表达式的C语言实现

[参考网站]( https://swtch.com/~rsc/regexp/regexp1.html )

## 转换为后缀形式

首先需要将输入的正则表达式转换为后缀形式，例如：

> "a(bb)+a"转换为"abb.+.a"。

**转换规则：**用''**.**''来表示连接。

## 转换为NFA

使用多个**State**结构体的链接表示NFA：

```C
struct State
{
	int c;
	State *out;
	State *out1;
	int lastlist;
};
```

