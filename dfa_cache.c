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
	
	// cat��ʾ�ж��ٸ������ӵ�Ƭ�Σ�cat���ᳬ��2
	// ������Ҫ����ǰ�����Ƿ���'.'������Ҫ�����������ַ�����
	// aa+����������������'aa+.' 
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
 * Ҫ�ǲ���Ptrlist�����union�Ļ�����Ӧ����ôд
 * struct Ptrlist{
 * 	 Ptrlist *next;
 *   State **s;
 * } 
 */

Ptrlist *list1(State **outp){
	/* �ظ�����ָ�����ڵĵ�ַ�ռ䣬ָ��δ��ֵʱ���ǹ���Ptrlist�ϵ� */
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

/* ����������ָ��������һ�� */
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
				// ����ע��Ҫ��e.outҲ�ӵ�����ָ��ļ�����ȥ 
				push(frag(s, append(e.out, list1(&s->out1))));
				break;
			default:
				s = state(*p, NULL, NULL);
				push(frag(s, list1(&s->out)));
				break;
		}
	}
	
	// ע��������е�����ָ�����ӵ���ƥ��״̬�� 
	e = pop();
	patch(e.out, &matchstate);
	return e.start;
#undef pop
#undef push
}

typedef struct List List;
struct List{
	// ��ʵ���Ǹ�Set 
	State **s;
	int n;
};
List l1, l2;
static int listid;

void addstate(List *l, State *s);
void step(List *clist, int c, List *nlist);

/* ��ʼ��һ�����ϣ����������һ����ʼ״̬start */
List *startlist(State *start, List *l){
	l->n = 0;
	listid++;
	addstate(l, start);
	return l;
}

/* ����������ӷ�Split��״̬ */
void addstate(List *l, State *s){
	if(s == NULL || s->lastlist == listid)
		return;
	
	s->lastlist = listid;
	if(s->c == Split){
		/* Split��ʵ�Ǹ���״̬������ת�Ƶ� */
		addstate(l, s->out);
		addstate(l, s->out1);
		return;
	}
	
	l->s[l->n++] = s;
}

/* ��������Ƿ�պ�ƥ�䵽matchstate */
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
	
	/* ÿǰ��һ������һ���µ�״̬���� */	
	listid++;
	nlist->n = 0;
	for(i = 0; i < clist->n; i++){
		s = clist->s[i];
		if(s->c == c){
			addstate(nlist, s->out);
		}
	}
}

/* dfa��ش��� */
typedef struct DState DState;
struct DState{
	List l;
	DState *next[256];
	DState *left;
	DState *right;
};

/* Compare pointers by address. */
int ptrcmp(const void *a, const void *b){
	if(a < b)
		return -1;
	if(a > b)
		return 1;
	return 0;
}

/* Compare lists: first by length, then by members. */
static int listcmp(List *l1, List *l2){
	int i;
	
	if(l1->n < l2->n)
		return -1;
	if(l1->n > l2->n)
		return 1;
	
	for(i = 0; i < l1->n; i++){
		if(l1->s[i] < l2->s[i])
			return -1;
		else if(l1->s[i] > l2->s[i])
			return 1;
	}
	
	return 0;
}

static DState *alldstate;	//�������ĸ��ڵ� 
static int ndstate;

static DState *freelist;

/*�ӻ���������л��DState*/
DState *allocdstate(){
	DState *d;
	
	if((d = freelist) != NULL){
		freelist = freelist->left;
	} else{
		d = malloc(sizeof(*d) + nstate*sizeof(State*));
		d->l.s = (State**)(d+1);
	}
	
	d->left = NULL;
	d->right = NULL;
	d->l.n = 0;
	memset(d->next, 0, sizeof(d->next));
	return d;
}

/* �ͷŶ��������е�DState��㣬�������ڴ棬�ظ����� */
void freestates(DState *d){
	if(d == NULL)
		return;
	freestates(d->left);
	freestates(d->right);
	/* �ҵ�freelist���� */
	d->left = freelist;
	freelist = d;
}

void freecache(){
	freestates(alldstate);
	alldstate = NULL;
	ndstate = 0;
}

/*
 * ��������֮ǰ�Ѿ�������DState��㣬��Ҫ������ʱֱ�ӷ��� 
 * �Զ���������ʽ���� 
 * Return the cached DState for list l,
 * creating a new one if needed.
 */
int maxstates = 32;
// nextp����ǰ�ڵ��nextָ�룬����ڴ治���Ļ�����ǰ�ڵ��nextָ���ʧЧ�� 
DState *dstate(List *l, DState **nextp){
	int i;
	DState **dp, *d;
	
	qsort(l->s, l->n, sizeof l->s[0], ptrcmp);
	dp = &alldstate;
	while((d = *dp) != NULL){
		i = listcmp(l, &d->l);
		if(i < 0)
			dp = &d->left;
		else if(i > 0)
			dp = &d->right;
		else
			return d;
	}
	
	if(ndstate >= maxstates){
		freecache();
		dp = &alldstate;
		nextp = NULL;
	}
	
	/* allocate, initialize new DState */
	d = allocdstate();
	memmove(d->l.s, l->s, l->n*sizeof(l->s[0]));
	d->l.n = l->n;
	
	/* insert in tree */
	*dp = d;
	
	ndstate++;
	if(nextp != NULL){
		/* �������Ľ���㹻�������� */
		*nextp = d;
	}
	
	return d;
}

DState *nextstate(DState *d, int c){
	step(&d->l, c, &l1);
	//��Ϊ���ڿ����ڴ����½���ʱ���ͷ����ϵĽ��
	//����next���ڵ��ڴ��Ѿ����ͷ��ˣ��������� 
	//return d->next[c] = dstate(&l1);
	return dstate(&l1, &d->next[c]);
}

DState *startDstate(State *start){
	return dstate(startlist(start, &l1), NULL);
}

int match(DState *start, char *s){
	int c;
	DState *d, *next;
	
	d = start;
	for(; *s; s++){
		c = *s & 0xFF;
		if((next = d->next[c]) == NULL)
			next = nextstate(d, c);
		d = next;
	}
	
	return ismatch(&d->l);
}

int main(){
	//reg2post("a(bb)+a|ab|ac");
	//abb.+.a.
	//printf("%s\n", reg2post("a(bb)+a"));
	
	char *post = reg2post("a?a?a?a?a?a?a?a?a?a?a?a?a?a?"
	"a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?");
	printf("%s\n", post);
	State *nfa = post2nfa(post);
	l1.s = malloc(nstate*sizeof(State*));
	l2.s = malloc(nstate*sizeof(State*));
	
	if(match(startDstate(nfa), "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")){
		printf("Match ok!");
	}
	
	return 0;
}
