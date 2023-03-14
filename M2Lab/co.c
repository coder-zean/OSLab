#include "co.h"





struct co dummy_co_wait_list_head;
struct co dummy_co_wait_list_tail;

// 创建main函数相关的协程
extern int main();
Co main_co = {"main", (void(*)(void*))main, 0, Run, 0, 0};
Co* co_in_progress = &main_co;


void RemoveListNode(Co* node) {
    assert(node);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = 0;
    node->prev = 0;
}

Co* PopListHead() {
    Co* head = dummy_co_wait_list_head.next;
    if (head == 0) return 0;
    dummy_co_wait_list_head.next = head->next;
    head->next->prev = &dummy_co_wait_list_head;
    head->next = 0;
    head->prev = 0;
    return head;
}

void InsertListTail(Co* node) {
    if (dummy_co_wait_list_head.next == 0) {
        dummy_co_wait_list_head.next = node;
        dummy_co_wait_list_tail.prev = node;
        node->prev = &dummy_co_wait_list_head;
        node->next = &dummy_co_wait_list_tail;
    } else {
        dummy_co_wait_list_tail.prev->next = node;
        node->prev = dummy_co_wait_list_tail.prev;
        node->next = &dummy_co_wait_list_tail;
        dummy_co_wait_list_tail.prev = node;
    }
}

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; callq *%1"
      : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#else
    "movl %0, %%esp; movl %2, 4(%0); callq *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#endif
  );
  if (co_in_progress->wait) {
    longjmp(co_in_progress->wait->co_yield_place, 1);
  } else {
    Co* head = PopListHead();
    if (!head)  return;
    co_in_progress->co_status = Finish;
    co_in_progress = head;
    if (head->co_status == Create) {
        head->co_status = Run;
        stack_switch_call(head->stack + MAX_STACK_SIZE / 2, (void*)head->func, (uintptr_t)head->func_arg);
    } else {
        longjmp(head->co_yield_place, 1);
    }
  }
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    Co* new_co = (Co*)malloc(sizeof(Co));
    new_co->co_name = name;
    new_co->func = func;
    new_co->func_arg = arg;
    new_co->co_status = Create;
    memset(new_co->stack, 0, MAX_STACK_SIZE);
    InsertListTail(new_co);
    return new_co;
}

void co_wait(struct co *co) {
    if (co->co_status == Finish) {
      free(co);
      return;
    }
    int jmp_flag = setjmp(co_in_progress->co_yield_place);
    if (jmp_flag == 0) {
        RemoveListNode(co);
        co->wait = co_in_progress;
        co_in_progress = co;
        if (co->co_status == Create) {
            co->co_status = Run;
            stack_switch_call(co->stack + MAX_STACK_SIZE / 2, (void*)co->func, (uintptr_t)co->func_arg);
        } else {
            longjmp(co->co_yield_place, 1);
        }
        return;
    } 
    co_in_progress->co_status = Finish;
    Co* wait_co = co->wait;
    free(co_in_progress);
    co_in_progress = wait_co;
}

void co_yield() {
    int jmp_flag = setjmp(co_in_progress->co_yield_place);
    if (!jmp_flag) {
        Co* head = PopListHead();
        if (!head)  return;
        InsertListTail(co_in_progress);
        co_in_progress = head;
        if (head->co_status == Create) {
            head->co_status = Run;
            stack_switch_call(head->stack + MAX_STACK_SIZE / 2, (void*)head->func, (uintptr_t)head->func_arg);
        } else {
            longjmp(head->co_yield_place, 1);
        }
    }
}





