#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
//-lpthread
#include <unistd.h>

// TODO clean up memory

typedef pthread_t thread;

typedef struct {
  void** stack;
  int length;
  int allocated;
  pthread_mutex_t lock;
} safeStack;

safeStack* newSafeStack() {
  safeStack* ret = (safeStack*)malloc(sizeof(safeStack));
  if (ret) {
    pthread_mutex_lock(&ret->lock);
    int initialLength = 16;//1024; for debugging keep intitial short to ensure it works
    ret->stack = (void**)malloc(initialLength*sizeof(void*));
    ret->allocated = initialLength;
    pthread_mutex_unlock(&ret->lock);
  }
  return ret;
}

void safeStackPush(safeStack* ss, void* elt) {
  pthread_mutex_lock(&ss->lock);
  if (ss->length >= ss->allocated) {
    void** newStack = (void**)malloc(2*ss->allocated*sizeof(void*));
    for (int i=0; i<ss->length; ++i) { // use copy
      newStack[i] = ss->stack[i]; 
    }
    ss->stack = newStack;
    ss->allocated = 2*ss->allocated;
  }
  ss->stack[ss->length] = elt;
  ss->length++;
  pthread_mutex_unlock(&ss->lock);
}

void* safeStackPop(safeStack* ss) {
  void* ret;
  bool found = false;
  while(found == false) {
    pthread_mutex_lock(&ss->lock);
    if (ss->length > 0) {
      ret = ss->stack[ss->length-1];
      ss->length--;
      found = true;
    }  
    pthread_mutex_unlock(&ss->lock);
  }
  return ret;
}

typedef struct {
  safeStack* stack; 
} threadStore;

threadStore* newThreadStore(int max) {
  threadStore* ret = (threadStore*)malloc(sizeof(threadStore));
  if (ret) {
    ret->stack = newSafeStack();
    for (int i = 0; i < max; ++i) {
      thread* tid = (thread*)malloc(sizeof(thread*));
      safeStackPush(ret->stack, tid);
    }
  }
  return ret;
}

thread* getThread(threadStore* ts) {
  thread* ret = (thread*)safeStackPop(ts->stack);
  return ret;
}

typedef struct {
  thread* t;
  safeStack* ss;
} threadPacket;

void* fn(void* ptr) {
  // do stuff 
  // ...
  //
  threadPacket* tp = (threadPacket*)ptr;

  sleep(1); // 0.1 is possible
  // last step is put back into stack
  safeStackPush(tp->ss, tp->t);
}

void run(thread* t, void* (fn)(void* ptr), safeStack* ss) {
  threadPacket* tp = (threadPacket*)malloc(sizeof(threadPacket));
  tp->t = t;
  tp->ss = ss;
  pthread_create(t, NULL, fn, (void*)tp);
}

int main() {
  printf("hola\n");

  int MAX_THREADS = 1000;
  int TOTAL = 10000;

  threadStore* ts = newThreadStore(MAX_THREADS); 

  thread** threads = (thread**)malloc(TOTAL*sizeof(thread*));
  int idx = 0;
  for (int i = 0; i < TOTAL; ++i) {
    thread* t = getThread(ts);
    run(t, fn, ts->stack);
    idx++;
  }
}
