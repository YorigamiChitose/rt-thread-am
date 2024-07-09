#include <rtdef.h>
#include <am.h>
#include <klib.h>
#include <rtthread.h>

typedef void (*tentry_t) (void *);
typedef void (*texit_t) (void);
typedef struct {
  tentry_t entry;
  texit_t exit;
  void *args;
} Args;
typedef struct {
  rt_ubase_t from;
  rt_ubase_t to;
} Switch_info;

static Context* switch_handler(Context *c) {
  rt_thread_t c_thread = rt_thread_self();
  Switch_info *switch_info = (Switch_info *)c_thread->user_data;
  if (switch_info->from) {
    *(Context **)switch_info->from = c;
  }
  return *(Context **)switch_info->to;
}

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
      c = switch_handler(c);
      break;
    case EVENT_IRQ_TIMER:
      break;
    case EVENT_IRQ_IODEV:
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_thread_t c_thread = rt_thread_self();
  rt_ubase_t user_data_cache = c_thread->user_data;
  Switch_info switch_info = {from, to};
  c_thread->user_data = (rt_ubase_t)&switch_info;
  yield();
  c_thread->user_data = user_data_cache;
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_hw_context_switch(0, to);
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

void wrap_func(void * arg) {
  ((Args *)arg)->entry(((Args *)arg)->args);
  ((Args *)arg)->exit();
  assert(0);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  Area area = {.start = NULL, .end = (void *)((rt_ubase_t)(stack_addr + (((rt_ubase_t)stack_addr & 7) ? 8 : 0)) & (~7))};
  Args *args = rt_malloc(sizeof(Args));
  args->entry = tentry;
  args->exit = texit;
  args->args = parameter;
  Context *context = kcontext(area, wrap_func, args);
  return (rt_uint8_t *)context;
}
