#include<cstdio>
#include<cstdlib>
#include<iostream>

struct test{
    int a;
    int b;
};
int main(){
    struct test *t;
    t->a = 15;
    t->a = 14;
    struct test *buf = (struct test*)malloc(sizeof(struct test));
    buf[0] = *t;
    struct test * cas = (struct test *) buf;
    printf("struct a : %d\t struct b : %d", t->a, t->b);
    printf("a : %d\tb : %d", cas->a, cas->b);

}

