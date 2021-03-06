#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *reg2post(char *reg){
	char *p = reg;
	static char res[8000];
	char *q = res;
	struct {
		int cat; 	//concatenation 
		int alt;	//alternation 
	}stack[100];
	
	// cat表示有多少个待连接的片段，cat不会超过2
	// 但是需要绝对前两个是否用'.'连接需要看到第三个字符才行
	// aa+比如这种情况，输出'aa+.' 
	int cat = 0, alt = 0, top = -1;
	
	if(strlen(reg) >= sizeof(res)/2){
		printf("regular expression is too long.");
		return NULL;
	}
	
	for(; *p; p++){
		switch(*p){
			case '(':
				if(cat > 1){
					*q = '.';
					cat--;
				}
				if(++top >= 100){
					printf("too many brackets.");
					return NULL;
				}
				stack[top].cat = cat;
				stack[top].alt = alt;
				cat = 0;
				alt = 0;
				break;
			case ')':
				if(top == -1){
					printf("brackets are not matching.");
					return NULL;
				}
				if(cat == 0){
					printf("no expression in the brackets.");
					return NULL;
				}
				
				if(cat > 1){
					*q++ = '.';
					cat--;
				}
				while(alt-- > 0){
					*q++ = '|';
				}
				cat = stack[top].cat;
				alt = stack[top--].alt;
				cat++;
				break;
			case '|':
				if(cat == 0){
					printf("no expression before '|'");
					return NULL;
				}
				if(cat > 1){
					*q++ = '.';
				}
				cat = 0;
				alt++;
				break;
			case '*':
			case '+':
			case '?':
				if(cat == 0){
					printf("no expression before %s.", *p);
					return NULL;
				}
				*q++ = *p;
				break;
			default:
				if(cat > 1){
					*q++ = '.';
					cat--;
				}
				cat++;
				*q++ = *p;
				break;
		}
	}
	if(top != -1){
		printf("brackets are not matching.");
		return NULL;
	}
	
	if(cat > 1){
		*q++ = '.';
		cat--;
	}
	while(alt-- > 0){
		*q++ = '|';
	}
	*q = '\0';
	//printf("%s\n", res);
	return res;
}

/*
 * Represents an NFA state plus zero or one or two arrows exiting.
 * if c == Match, no arrows out; matching state.
 * If c == Split, unlabeled arrows to out and out1 (if != NULL).
 * If c < 256, labeled arrow with character c to out.
 */
enum{
	Match = 256,
	Split = 257
};
typedef struct State State;
struct State{
	int c;
	State *out;
	State *out1;
	int lastlist;
};
State matchstate = {Match};
int nstate;

State *state(int c, State *out, State *out1){
	State *s;
	
	nstate++;
	s = malloc(sizeof *s);
	s->c = c;
	s->out = out;
	s->out1 = out1;
	s->lastlist = 0;
	return s;
}

typedef union Ptrlist Ptrlist;
union Ptrlist{
	Ptrlist *next;
	State *s;
};

/*
 * 要是不把Ptrlist定义成union的话，就应该这么写
 * struct Ptrlist{
 * 	 Ptrlist *next;
 *   State **s;
 * } 
 */

Ptrlist *list1(State **outp){
	/* 重复利用指针所在的地址空间，指针未赋值时就是挂在Ptrlist上的 */
	Ptrlist *l;
	
	l = (Ptrlist *)outp;
	l->next = NULL;
	return l;
}

/* Patch the list of states at out to point to start. */
Ptrlist *patch(Ptrlist *l, State *s){
	Ptrlist *next;
	
	for(; l; l=next){
		next = l->next;
		l->s = s;
	}
}

/* 将两组悬挂指针连接在一起 */
Ptrlist *append(Ptrlist *l1, Ptrlist *l2){
	Ptrlist *oldl1;
	
	oldl1 = l1;
	while(l1->next){
		l1 = l1->next;
	}
	l1->next = l2;
	return oldl1;
}

typedef struct Frag Frag;
struct Frag{
	State *start;
	Ptrlist *out;
};
Frag frag(State *start, Ptrlist *out){
	Frag n = { start, out };
	return n;
}

/*
 * Convert postfix regular expression to NFA.
 * Return start state.
 */
State *post2nfa(char *postfix){
	char *p;
	Frag stack[1000], *stackp, e1, e2, e;
	State *s;
	Ptrlist *l;
	
	#define push(s) *stackp++ = s
	#define pop()   *--stackp
	
	stackp = stack;
	for(p = postfix; *p; p++){
		switch(*p){
			case '.':
				e2 = pop();
				e1 = pop();
				patch(e1.out, e2.start);
				push(frag(e1.start, e2.out));
				break;
			case '|':
				e2 = pop();
				e1 = pop();
				s = state(Split, e1.start, e2.start);
				push(frag(s, append(e1.out, e2.out)));
				break;
			case '+':	// one or more
				e = pop();
				s = state(Split, e.start, NULL);
				patch(e.out, s);
				push(frag(e.start, list1(&s->out1)));
				break;
			case '*':	// zero or more
				e = pop();
				s = state(Split, e.start, NULL);
				patch(e.out, s);
				push(frag(s, list1(&s->out1)));
				break;
			case '?':	// zero or one 
				e = pop();
				s = state(Split, e.start, NULL);
				// 这里注意要将e.out也加到悬挂指针的集合里去 
				push(frag(s, append(e.out, list1(&s->out1))));
				break;
			default:
				s = state(*p, NULL, NULL);
				push(frag(s, list1(&s->out)));
				break;
		}
	}
	
	// 注意最后将所有的悬挂指针连接到“匹配状态” 
	e = pop();
	patch(e.out, &matchstate);
	return e.start;
#undef pop
#undef push
}

typedef struct List List;
struct List{
	// 其实就是个Set 
	State **s;
	int n;
};
List l1, l2;
static int listid;

void addstate(List *l, State *s);
void step(List *clist, int c, List *nlist);

/* 初始化一个集合，往里面加入一个初始状态start */
List *startList(State *start, List *l){
	l->n = 0;
	listid++;
	addstate(l, start);
	return l;
}

/* 往集合里添加非Split的状态 */
void addstate(List *l, State *s){
	if(s == NULL || s->lastlist == listid)
		return;
	
	s->lastlist = listid;
	if(s->c == Split){
		/* Split其实是个空状态，负责转移的 */
		addstate(l, s->out);
		addstate(l, s->out1);
		return;
	}
	
	l->s[l->n++] = s;
}

/* start是nfa的起始状态，s是要匹配的字符串 */
int match(State *start, char *s){
	List *clist, *nlist, *t;
	clist = startList(start, &l1);
	nlist = &l2;
	for(; *s; s++){
		step(clist, *s, nlist);
		t = clist; clist = nlist; nlist = t;
	}
	return ismatch(clist);
}

/* 检测最终是否刚好匹配到matchstate */
int ismatch(List *l){
	int i;
	
	for(i = 0; i < l->n; i++){
		if(l->s[i] == &matchstate){
			return 1;
		}
	}
	return 0;
}

void step(List *clist, int c, List *nlist){
	int i;
	State *s;
	
	/* 每前进一步产生一组新的状态集合 */	
	listid++;
	nlist->n = 0;
	for(i = 0; i < clist->n; i++){
		s = clist->s[i];
		if(s->c == c){
			addstate(nlist, s->out);
		}
	}
}

int main(){
	//reg2post("a(bb)+a|ab|ac");
	//abb.+.a.
	//printf("%s\n", reg2post("a(bb)+a"));
	char *post = reg2post("a+b");
	printf("%s\n", post);
	State *nfa = post2nfa(post);
	l1.s = malloc(nstate*sizeof(State*));
	l2.s = malloc(nstate*sizeof(State*));
	if(match(nfa, "b")){
		printf("match ok!");
	}
}
