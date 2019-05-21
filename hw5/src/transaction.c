#include "server.h"
#include "csapp.h"
#include "transaction.h"
#include <debug.h>
#include <pthread.h>
#include <semaphore.h>


TRANSACTION trans_list;
unsigned int newId = 0;

/*
 * Initialize the transaction manager.
 */
void trans_init(void) {
  trans_list.id = newId++;                              // Transaction ID.
  trans_list.refcnt = 0;                          // Number of references (pointers) to transaction.
  trans_list.status = TRANS_PENDING;                            // Current transaction status.
  trans_list.depends = NULL;                      // Singly-linked list of dependencies.
  trans_list.waitcnt = 0;                         // Number of transactions waiting for this one.
  Sem_init(&(trans_list.sem), 0, 0);              // Semaphore to wait for transaction to commit or abort.
  pthread_mutex_init(&(trans_list.mutex), NULL);  // Mutex to protect fields.
  trans_list.next = &trans_list;                  // Next in list of all transactions
  trans_list.prev = &trans_list;                   // PREV
}

/*
 * Finalize the transaction manager.
 */
void trans_fini(void) {
  //Free transaction
  debug("Finalize the transaction manager.");

}

/*
 * Create a new transaction.
 *
 * @return  A pointer to the new transaction (with reference count 1)
 * is returned if creation is successful, otherwise NULL is returned.
 */
TRANSACTION *trans_create(void) {
  TRANSACTION* newTrans = Calloc(1, sizeof(TRANSACTION));
  newTrans->id = newId++;
  newTrans->refcnt = 1;
  newTrans->status = TRANS_PENDING;
  newTrans->depends = NULL;
  newTrans->waitcnt = 0;
  Sem_init(&(newTrans->sem), 0, 0);
  pthread_mutex_init(&(newTrans->mutex), NULL);
  newTrans->next = trans_list.next;
  newTrans->prev = &(trans_list);
  trans_list.next->prev = newTrans;
  trans_list.next = newTrans;

  return newTrans;
}

/*
 * Increase the reference count on a transaction.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the increase.
 * @return  The transaction pointer passed as the argument.
 */
TRANSACTION *trans_ref(TRANSACTION *tp, char *why) {
  pthread_mutex_lock(&(tp->mutex));
  tp->refcnt++;
  debug("%s", why);
  pthread_mutex_unlock(&(tp->mutex));
  return tp;
}

/*
 * Decrease the reference count on a transaction.
 * If the reference count reaches zero, the transaction is freed.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the decrease.
 */
void trans_unref(TRANSACTION *tp, char *why) {
  pthread_mutex_lock(&(tp->mutex));
  tp->refcnt--;
  if (tp->refcnt == 0) {
    //find transaction in translist, free dependency, then free transactions
    TRANSACTION* trans_ptr = &trans_list;
    while (trans_ptr->next != tp || trans_ptr->next != &(trans_list)) {
      trans_ptr = trans_ptr->next;
    }
    trans_ptr->next->prev = trans_ptr->prev;
    trans_ptr->prev->next = trans_ptr->next;
    trans_ptr->next = NULL;
    trans_ptr->prev = NULL;
    DEPENDENCY* depend_ptr = trans_ptr->depends;
    while (depend_ptr->next != NULL) {
      DEPENDENCY* tempDepend = depend_ptr;
      depend_ptr = depend_ptr->next;
      Free(tempDepend);
    }
    if (depend_ptr != NULL)
      Free(depend_ptr);
    Free(tp);
  }
  debug("%s", why);
  pthread_mutex_unlock(&(tp->mutex));

}

/*
 * Add a transaction to the dependency set for this transaction.
 *
 * @param tp  The transaction to which the dependency is being added.
 * @param dtp  The transaction that is being added to the dependency set.
 */
void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp) {

  DEPENDENCY* newDepend = Malloc(sizeof(DEPENDENCY));
  newDepend->trans = tp;
  newDepend->next = NULL;

  //go through dependency list
  if (dtp->depends != NULL) {
    DEPENDENCY* depend_ptr = dtp->depends;
    while(depend_ptr->next != NULL || (depend_ptr->trans == tp)) {
      depend_ptr = depend_ptr->next;
    }
    if(depend_ptr->trans == tp)
      return;

    depend_ptr->next = newDepend;
  } else {
    dtp->depends = newDepend;
    
  }

  trans_ref(tp, "Adding dependency");
  pthread_mutex_lock(&(tp->mutex));
  tp->waitcnt++;        // *******************NOT SURE IF I SHOULD DO THIS??
  pthread_mutex_unlock(&(tp->mutex));

}

/*
 * Try to commit a transaction.  Committing a transaction requires waiting
 * for all transactions in its dependency set to either commit or abort.
 * If any transaction in the dependency set abort, then the dependent
 * transaction must also abort.  If all transactions in the dependency set
 * commit, then the dependent transaction may also commit.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be committed.
 * @return  The final status of the transaction: either TRANS_ABORTED,
 * or TRANS_COMMITTED.
 */
TRANS_STATUS trans_commit(TRANSACTION *tp) {

  // debug("Trying to commit transaction %d", tp->id);

  DEPENDENCY* depend_ptr = tp->depends;

  while(tp->waitcnt != 0) {
    if (tp->status == TRANS_ABORTED)
      return trans_abort(tp);

    P(&(tp->sem));
    pthread_mutex_lock(&(tp->mutex));
    tp->waitcnt--;
    pthread_mutex_unlock(&(tp->mutex));
  }

  if (tp->depends != NULL) {
    while(depend_ptr->next != NULL) {
      V(&(depend_ptr->trans->sem));
      depend_ptr = depend_ptr->next;
    }

    V(&(depend_ptr->trans->sem));

    pthread_mutex_lock(&(tp->mutex));
    tp->status = TRANS_COMMITTED;
    pthread_mutex_unlock(&(tp->mutex));

    trans_unref(tp, NULL);
    return TRANS_COMMITTED;
  }

  // debug("Transaction %d commited", tp->id);
  return TRANS_COMMITTED;
}

/*
 * Abort a transaction.  If the transaction has already committed, it is
 * a fatal error and the program crashes.  If the transaction has already
 * aborted, no change is made to its state.  If the transaction is pending,
 * then it is set to the aborted state, and any transactions dependent on
 * this transaction must also abort.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be aborted.
 * @return  TRANS_ABORTED.
 */
TRANS_STATUS trans_abort(TRANSACTION *tp) {
  if (tp->status == TRANS_COMMITTED) {
    abort();
  } else if (tp->status == TRANS_ABORTED) {
    return TRANS_ABORTED;
  } else {  //TRANS_PENDING
    pthread_mutex_lock(&(tp->mutex));
    tp->status = TRANS_ABORTED;

    if (tp->depends != NULL) {
      DEPENDENCY* depend_ptr = tp->depends;
      while(depend_ptr->next != NULL) {
        depend_ptr->trans->status = TRANS_ABORTED;
        tp->waitcnt--;
        trans_unref(depend_ptr->trans, NULL);
        depend_ptr = depend_ptr->next;
      }

      depend_ptr->trans->status = TRANS_ABORTED;
      trans_abort(depend_ptr->trans);
    }
    pthread_mutex_unlock(&(tp->mutex));
    return TRANS_ABORTED;
  }
}

/*
 * Get the current status of a transaction.
 * If the value returned is TRANS_PENDING, then we learn nothing,
 * because unless we are holding the transaction mutex the transaction
 * could be aborted at any time.  However, if the value returned is
 * either TRANS_COMMITTED or TRANS_ABORTED, then that value is the
 * stable final status of the transaction.
 *
 * @param tp  The transaction.
 * @return  The status of the transaction, as it was at the time of call.
 */
TRANS_STATUS trans_get_status(TRANSACTION *tp) {
  return tp->status;
}

/*
 * Print information about a transaction to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 *
 * @param tp  The transaction to be shown.
 */
void trans_show(TRANSACTION *tp) {

  fprintf(stderr, "TRANSACTIONS:\n");
  fprintf(stderr, "[id=%d, status=%d, refcnt=%d]\n", tp->id, tp->status, tp->refcnt);
}

/*
 * Print information about all transactions to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 */
void trans_show_all(void) {
  TRANSACTION* trans_ptr = (trans_list.next);
  fprintf(stderr, "TRANSACTIONS:\n");
  while(trans_ptr != &trans_list) {
    fprintf(stderr, "[id=%d, status=%d, refcnt=%d]", trans_ptr->id, trans_ptr->status, trans_ptr->refcnt);
    trans_ptr = trans_ptr->next;
  }

  fprintf(stderr, "\n");
}
