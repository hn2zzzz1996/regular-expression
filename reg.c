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
			case '+':
				e = pop();
				s = state(Split, e.start, NULL);
				patch(e.out, s);
				push(frag(e.start, list1(&s->out1)));
			case '*':
				e = pop();
				s = state(Split, e.start, NULL);
				patch(e.out, s);
				push(frag(s, list1(&s->out1)));
				break;
			case '?':
				e = pop();
				s = state(Split, e.start, NULL);
				push(frag(s, list1(&s->out1)));
				break;
			default:
				s = state(*p, NULL, NULL);
				push(frag(s, list1(&s->out)));
				break;
		}
	}
	
	e = pop();
	patch(e.out, &matchstate);
	return e.start;
#undef pop
#undef push
}

typedef struct List List;
struct List{
	State **s;
	int n;
};
List l1, l2;
static int listid;

List *startList(State *start, List *l){
	l->n = 0;
	listid++;
	addstate(l, start);
	return l;
}

void addstate(List *l, State *s){
	if(s == NULL || s->lastlist == listid)
		return;
		
	l->s[l->n++] = s;
}

int main(){
	reg2post("a(bb)+a|ab|ac");
	//abb.+.a.
	printf("%s\n", reg2post("a(bb)+a"));
}
