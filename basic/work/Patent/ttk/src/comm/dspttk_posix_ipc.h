
#ifndef _DSPTTK_POSIX_IPC_H_
#define _DSPTTK_POSIX_IPC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sal_type.h"
#include "sal_macro.h"


/*======================== POSIX线程 ========================*/
/**
 * Minimum supported stack size for a thread
 * Some OS may not define this PTHREAD_STACK_MIN, we choose 16K for min stack size.
 */
#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN		16384
#endif

/**
 * 线程的调度策略
 */
typedef enum
{
    DSPTTK_PTHREAD_SCHED_OTHER,     // 另外的调度策（根据实现定义）。这是任何新创建线程的默认调度策略
    DSPTTK_PTHREAD_SCHED_RR,        // 轮询调度策略，按照时间片将每个线程分配到处理器上
    DSPTTK_PTHREAD_SCHED_FIFO,      // 先进先出调度策略，执行线程运行到结束
} DSPTTK_PTHREAD_SCHED_POLICY;

/**
 * @fn      dspttk_pthread_spawn
 * @brief   创建线程，线程默认属性为：：绑定轻进程，分离，调度属性不继承 
 * @warning 注：不允许调用pthread_join获得另一个线程的退出状态，因为是设置了进程的分离性 
 * 
 * @param   thread_id[IN] 线程ID号，可为NULL，typedef unsigned long pthread_t 
 * @param   policy[IN] 线程调度策略
 * @param   priority[IN] 优先级，值越大，优先级越高，取值范围[0, 100]
 * @param   stacksize[IN] 栈空间，至少为16KB，建议大小为1MB，即0x100000
 * @param   function[IN] 函数名，线程入口
 * @param   argc[IN] 函数参数个数，取值范围[0, 10]，后面紧跟函数形参
 * 
 * @return  SAL_SOK-创建线程成功，SAL_FAIL-创建线程失败
 */
SAL_STATUS dspttk_pthread_spawn(pthread_t *thread_id, DSPTTK_PTHREAD_SCHED_POLICY policy, int priority, size_t stacksize, void *function, unsigned int argc, ...);

/**
 * @fn      dspttk_pthread_set_name
 * @brief   设置线程名
 * 
 * @param   thread_name[IN] 线程名，注：加最后一个空字符'\0'，总长不能超过16个字符
 * 
 * @return  SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
SAL_STATUS dspttk_pthread_set_name(char *thread_name);

/**
 * @fn      dspttk_pthread_get_name
 * @brief   获取线程名
 * 
 * @param   thread_name[OUT] 线程名
 * @param   name_len[IN] 线程名字符串长度，不能小于16
 * 
 * @return  SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
SAL_STATUS dspttk_pthread_get_name(char *thread_name, size_t name_len);

/**
 * @fn      dspttk_pthread_get_selfid
 * @brief   获取线程自身的ID号 
 * @note    该函数执行总是成功，没有失败
 *  
 * @return  线程自身的ID号，pthread_t的类型为unsigned long int
 */
pthread_t dspttk_pthread_get_selfid(void);

/**
 * @fn      dspttk_pthread_is_alive
 * @brief   线程是否活着
 * 
 * @param   thread_id[IN] 线程ID号
 * 
 * @return  TRUE-线程活着，FALSE-线程不存在
 */
BOOL dspttk_pthread_is_alive(pthread_t thread_id);

/**
 * 互斥量是用于上锁，条件变量用于等待！！！
 */

/*======================= POSIX互斥锁 =======================*/
/**
 * @fn      dspttk_mutex_init
 * @brief   初始化互斥锁，默认类型位PTHREAD_MUTEX_ERRORCHECK，即防止最简单的死锁发生
 * @note    如果同一线程企图对已上锁的mutex进行relock，或企图对未上锁的mutex进行unlock，均返回错误 
 *  
 * @param   mutex_id[OUT] 互斥锁ID
 * 
 * @return  SAL_SOK-初始化互斥锁成功，SAL_FAIL-初始化互斥锁失败
 */
SAL_STATUS dspttk_mutex_init(pthread_mutex_t *mutex_id);

/**
 * @fn      dspttk_mutex_init_ex
 * @brief   初始化互斥锁，并带有互斥锁类型
 *  
 * @param   mutex_id[OUT] 互斥锁ID
 * @param   mutex_type[IN] 互斥锁类型：分为PTHREAD_MUTEX_NORMAL、PTHREAD_MUTEX_ERRORCHECK、 
 *                                      PTHREAD_MUTEX_RECURSIVE和PTHREAD_MUTEX_DEFAULT 
 *  
 * @return  SAL_SOK-初始化互斥锁成功，SAL_FAIL-初始化互斥锁失败
 */
SAL_STATUS dspttk_mutex_init_ex(pthread_mutex_t *mutex_id, int mutex_type);

/**
 * @fn      dspttk_mutex_lock
 * @brief   尝试在一定时间内对mutex_id上锁
 * 
 * @param   mutex_id[IN] 互斥锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS dspttk_mutex_lock(pthread_mutex_t *mutex_id, long wait_ms, const char *function, const unsigned int line);

/**
 * @fn      dspttk_mutex_unlock
 * @brief   对mutex_id解锁
 * 
 * @param   mutex_id[IN] 互斥锁ID
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_mutex_unlock(pthread_mutex_t *mutex_id, const char *function, const unsigned int line);

/**
 * @fn      dspttk_mutex_destroy
 * @brief   销毁互斥锁
 * 
 * @param   mutex_id[IN] 互斥锁ID
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_mutex_destroy(pthread_mutex_t *mutex_id);


/*======================= POSIX条件变量 =======================*/
typedef struct
{
	pthread_mutex_t mid;	// 互斥锁ID
	pthread_cond_t cid;		// 条件变量ID
}DSPTTK_COND_T;

/**
 * @note condWait使用说明：
 * mutexLock(&cond_s->mid, FOREVER); // 加锁
 * while (等待条件) // 比如等待条件为队列空
 * {
 *     condWait(cond_s, wait_ms, __FUNCTION__, __LINE__);
 * }
 * TODO: 对变量（一般是全局变量）进行操作 // 比如从队列中获取并删除一个元素
 * mutexUnlock(&cond_s->mid); // 解锁
 */

/**
 * @note condSignal使用说明：
 * mutexLock(&cond_s->mid, FOREVER); // 加锁
 * TODO: 对变量（一般是全局变量）进行操作 // 比如向队列中插入一个元素
 * condSignal(cond_s, true, __FUNCTION__, __LINE__);
 * mutexUnlock(&cond_s->mid); // 解锁
 */

/**
 * @fn      dspttk_cond_init
 * @brief   初始化条件变量
 * 
 * @param   cond_s[OUT] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * 
 * @return  SAL_SOK-初始化成功，SAL_FAIL-初始化失败
 */
SAL_STATUS dspttk_cond_init(DSPTTK_COND_T *cond_s);

/**
 * @fn      dspttk_cond_wait
 * @brief   在一定时间内等待条件变为真
 * @warning dspttk_cond_wait成功返回时，线程需要重新计算条件，因为其他线程可能在运行过程中已经改变条件
 *
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * @param   wait_ms[IN] 等待条件变为真的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，其他表示时间
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-等待条件变为真成功，SAL_FAIL-等待条件变成真失败
 */
SAL_STATUS dspttk_cond_wait(DSPTTK_COND_T *cond_s, long wait_ms, const char *function, const unsigned int line);

/**
 * @fn      dspttk_cond_signal
 * @brief   向等待条件的线程发送唤醒信号
 * 
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * @param   bSignalBoardcast[IN] 唤醒信号是否为广播类型：
 *              TRUR-广播条件状态的改变，以唤醒等待该条件的所有线程（常用）
 *              FASLE-只会唤醒等待该条件的某个线程
 *          注：只有在等待者代码编写确切，只有一个等待者需要唤醒，且唤醒哪个线程无所谓，
 *              那么此时为这种情况使用单播，所以其他情况下都必须使用广播发送。
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0 
 *  
 * @return  SAL_SOK-发送唤醒信号成功，SAL_FAIL-发送唤醒信号失败
 */
SAL_STATUS dspttk_cond_signal(DSPTTK_COND_T *cond_s, BOOL bSignalBoardcast, const char *function, const unsigned int line);

/**
 * @fn      dspttk_cond_destroy
 * @brief   销毁条件变量
 * 
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * 
 * @return  SAL_SOK-销毁成功，SAL_FAIL-销毁失败
 */
SAL_STATUS dspttk_cond_destroy(DSPTTK_COND_T *cond_s);


/*======================= POSIX读写锁 =======================*/
/**
 * @fn      rwlockInit
 * @brief   初始化读写锁，并设置写锁优先
 *  
 * @param   rwlock_id[OUT] 读写锁ID
 * 
 * @return  SAL_SOK-初始化读写锁成功，SAL_FAIL-初始化读写锁失败
 */
SAL_STATUS dspttk_rwlock_init(pthread_rwlock_t *rwlock_id);

/**
 * @fn      dspttk_rwlock_rdlock
 * @brief   尝试在一定时间内对rdlock上锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * 
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS dspttk_rwlock_rdlock(pthread_rwlock_t *rwlock_id, long wait_ms);

/**
 * @fn      dspttk_rwlock_wrlock
 * @brief   尝试在一定时间内对wrlock上锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * 
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS dspttk_rwlock_wrlock(pthread_rwlock_t *rwlock_id, long wait_ms);

/**
 * @fn      dspttk_rwlock_unlock
 * @brief   对rwlock_id解锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_rwlock_unlock(pthread_rwlock_t *rwlock_id);

/**
 * @fn      dspttk_rwlock_destroy
 * @brief   销毁读写锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_rwlock_destroy(pthread_rwlock_t *rwlock_id);


/*======================= POSIX信号量/共享内存 =======================*/
#define IPC_RDWR	0666	// 可读可写的权限值
#define IPC_RDONLY	0444	// 只读的权限值
#define IPC_WRONLY	0222	// 只写的权限值

/**
 * @fn      dspttk_sem_init
 * @brief   创建无名信号量，并为信号量赋初值
 * @warning 同一信号量不能调用两次semInit，若不再使用，需调用semDestroy销毁
 *
 * @param   sem[OUT] 无名信号量，注意是sem_t类型
 * @param   sem_value[IN] 无名信号量初始值
 *
 * @return  SAL_SOK-初始化成功；SAL_FAIL-初始化失败
 */
SAL_STATUS dspttk_sem_init(OUT sem_t *sem, IN unsigned int sem_value);

/**
 * @fn      dspttk_sem_destroy
 * @brief   销毁无名信号量
 * 
 * @param   sem[IN] 无名信号量，注意必须已初始化
 * 
 * @return  SAL_SOK-销毁成功；SAL_FAIL-销毁失败
 */
SAL_STATUS dspttk_sem_destroy(IN sem_t *sem);

/**
 * @fn      dspttk_sem_open
 * @brief   打开或创建一个有名信号量
 * 
 * @param   name[IN] 信号量名称，以“/”开头，且名字中不能包含其他的“/”，最长不超过_POSIX_NAME_MAX
 * @param   oflag[IN] 打开方式，参数可为0、O_CREAT、O_EXCL，
 *                    0：打开一个已存在的信号量，
 *                    O_CREAT：如果信号量不存在就创建一个信号量，如果存在则打开被返回，此时mode和value需要指定
 *                    O_CREAT|O_EXCL：如果信号量不存在就创建一个信号量，如果信号量已存在则返回错误
 * @param   mode[IN]  信号量的权限位，和open函数一样，包括：S_IRUSR，S_IWUSR，S_IRGRP，S_IWGRP，S_IROTH，S_IWOTH
 *                    常用的可以用宏定义IPC_RDWR、IPC_RDONLY、IPC_WRONLY
 * @param   value[IN] 创建信号量时，信号量的初始值，取值范围[0, SEM_VALUE_MAX] 
 *  
 * @return  NULL-打开或创建有名信号量失败；其他-打开或创建成功
 */
sem_t *dspttk_sem_open(IN const char *name, IN int oflag, ...);

/**
 * @fn      dspttk_sem_close
 * @brief   关闭一个有名信号量，但并没有把它从系统中删除
 * @note    从系统中删除有名信号量需调用接口semUnlink
 * 
 * @param   sem[IN] 信号量
 * 
 * @return  SAL_SOK-关闭有名信号量成功；SAL_FAIL-关闭失败
 */
SAL_STATUS dspttk_sem_close(IN sem_t *sem);

/**
 * @fn      dspttk_sem_unlink
 * @brief   从系统中删除有名信号量
 * 
 * @param   name[IN] 有名信号量名称，以“/”开头
 * 
 * @return  SAL_SOK-删除有名信号量成功；SAL_FAIL-删除失败
 */
SAL_STATUS dspttk_sem_unlink(const char *name);

/**
 * @fn      dspttk_sem_give
 * @brief   挂出一个信号量，该操作将信号量的值加1，若有进程阻塞着等待该信号量，则其中一个进程将被唤醒。
 * 
 * @param   sem[IN] 信号量，注意必须已初始化
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-信号量挂出成功；SAL_FAIL-信号量挂出失败，sem_post调用失败
 */
SAL_STATUS dspttk_sem_give(IN sem_t *sem, IN const char *function, IN const unsigned int line);


/**
 * @fn      dspttk_sem_take
 * @brief   等待一个信号量，通过wait_ms控制阻塞时间，若信号量大于0，等待进程将信号量的值减1，并获得共享资源的访问权限
 * 
 * @param   sem[IN] 信号量，注意必须已初始化
 * @param   wait_ms[IN] 等待信号量大于0的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-等待信号量成功，并将信号量值减1；SAL_FAIL-在wait_ms时间内未等待到信号量或sem_wait系列函数调用失败
 */
SAL_STATUS dspttk_sem_take(IN sem_t *sem, IN long wait_ms, IN const char *function, IN const unsigned int line);

/**
 * @fn      dspttk_sem_get_value
 * @brief   获取信号量的值
 * 
 * @param   sem[IN] 信号量，注意必须已初始化
 * 
 * @return  >=0：信号量的值，<0：获取失败
 */
INT32 dspttk_sem_get_value(IN sem_t *sem);

/**
 * @fn      dspttk_shm_open
 * @brief   通过共享内存标识符，打开或创建一个指定大小的共享内存
 * 
 * @param   key[IN] 共享内存标识符，实际为int型
 * @param   mem_size[IN] 共享内存大小，单位：字节
 * @param   flag[IN] 共享内存打开标识：(IPC_RDWR/IPC_RDONLY/IPC_WRONLY) | IPC_CREAT | IPC_EXCL
 * 
 * @return  非NULL：共享内存的指针，NULL：打开或创建失败
 */
void *dspttk_shm_open(key_t key, unsigned int mem_size, int flag);

/**
 * @fn      dspttk_shm_close
 * @brief   通过共享内存在进程中的虚拟地址，以关闭该共享内存
 * @note    将共享内存关闭并不是删除它，只是使该共享内存对当前进程不再可用
 *
 * @param   shm_addr[IN] 共享内存地址
 * 
 * @return  SAL_SOK-关闭共享内存成功；SAL_FAIL-关闭失败
 */
SAL_STATUS dspttk_shm_close(const void *shm_addr);

/**
 * @fn      dspttk_shm_destroy
 * @brief   删除共享内存段
 * 
 * @param   key[IN] 共享内存标识符，即shmOpen接口中的第一个参数
 * 
 * @return  SAL_SOK-删除共享内存成功；SAL_FAIL-删除失败
 */
SAL_STATUS dspttk_shm_destroy(const key_t key);


/*=================== Doubly Linked List ===================*/
typedef struct dnode
{
    struct dnode *next; /* pointer to the next node in the list */
    struct dnode *prev; /* pointer to the previous node in the list */
    void *pAdData;      /* additional data, always be a struct */
} DSPTTK_NODE;

typedef struct
{
    DSPTTK_NODE *head;  /* pointer to the head node in the list */
    DSPTTK_NODE *tail;  /* pointer to the tail node in the list */
    uint32_t count;     /* number of nodes in the list */
    DSPTTK_COND_T sync; /* the mutex and condition that control the list */
} DSPTTK_LIST;



/**
 * 注：以下接口，除lst_init外，均需在调用前加互斥锁，并在调用完成后释放。
 * 若连续调用多个接口，则可一起加锁，所有接口调用完成后一起释放。
 */

/**
 * @fn      dspttk_lst_get_head
 * @brief   peek the first node in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  a pointer to the first node, or NULL if the list is empty
 */
DSPTTK_NODE *dspttk_lst_get_head(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_get_tail
 * @brief   peek the last node in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  a pointer to the last node, or NULL if the list is empty
 */
DSPTTK_NODE *dspttk_lst_get_tail(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_is_full
 * @brief   judge the list is FULL
 * 
 * @param   list[IN] the list
 * 
 * @return  TRUE - full, FALSE - not full
 */
BOOL dspttk_lst_is_full(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_is_empty
 * @brief   judge the list is EMPTY
 * 
 * @param   list[IN] the list
 * 
 * @return  TRUE - empty, FALSE - not empty
 */
BOOL dspttk_lst_is_empty(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_get_count
 * @brief   get the number of nodes in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  the number of nodes in the list
 */
uint32_t dspttk_lst_get_count(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_get_idle_node
 * @brief   get an idle node in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  a pointer to an idle node, OR NULL if there's non idle node
 */
DSPTTK_NODE *dspttk_lst_get_idle_node(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_inst
 * @brief   insert a node in a list after a specified node
 * 
 * @param   list[IN] the list
 * @param   prev[IN] The new node is placed following the list node <prev>. 
 *                   If <prev> is NULL, the node is inserted at the head of the list. 
 * @param   node[IN] the new node
 */
void dspttk_lst_inst(DSPTTK_LIST *list, DSPTTK_NODE *prev, DSPTTK_NODE *node);

/**
 * @fn      dspttk_lst_push
 * @brief   push a node to the end of a list
 * 
 * @param   list[IN] the list
 * @param   node[IN] the new node
 */
void dspttk_lst_push(DSPTTK_LIST *list, DSPTTK_NODE *node);

/**
 * @fn      dspttk_lst_delete
 * @brief   delete a specified node from a list
 * 
 * @param   list[IN] the list
 * @param   node[IN] the node which needs to be deleted
 */
void dspttk_lst_delete(DSPTTK_LIST *list, DSPTTK_NODE *node);

/**
 * @fn      dspttk_lst_pop
 * @brief   delete the first node from a list
 * 
 * @param   list[IN] the list
 */
void dspttk_lst_pop(DSPTTK_LIST *list);

/**
 * @fn      dspttk_lst_search
 * @brief   search a node in a list and return the index of the node
 * @note    the first node's index is 0 
 *  
 * @param   list[IN] the list
 * @param   node[IN] the node needs to be searched
 * 
 * @return  The node index(form 0), or -1 if the node is not found
 */
int32_t dspttk_lst_search(DSPTTK_LIST *list, DSPTTK_NODE *node);

/**
 * @fn      dspttk_lst_locate
 * @brief   locate the index-th(from 0) node in a list
 * 
 * @param   list[IN] the list
 * @param   index[IN] the node which needs to be located,indx i as a strat
 * 
 * @return  a pointer to the index-th node, or NULL if non-existent
 */
DSPTTK_NODE *dspttk_lst_locate(DSPTTK_LIST *list, int32_t index);

/**
 * @fn      dspttk_lst_nstep
 * @brief   find a list node <n_step> steps away from a specified node
 * 
 * @param   node[IN] the node
 * @param   n_step[IN] If <n_step> is positive, it steps toward the tail
 *                     If <n_step> is negative, it steps toward the head
 * 
 * @return  a pointer to the node <n_step> steps away, or NULL if the node is out of range
 */
DSPTTK_NODE *dspttk_lst_nstep(DSPTTK_NODE *node, int32_t n_step);

/**
 * @fn      dspttk_lst_init
 * @brief   initializes a statistic list includes <node_num> nodes
 * 
 * @param   node_num[IN] the number of nodes in the list
 * 
 * @return  a pointer to a new list, OR NULL if init failed
 */
DSPTTK_LIST *dspttk_lst_init(uint32_t node_num);

/**
 * @fn      dspttk_lst_deinit
 * @brief   turn all nodes to NULL, and free the list
 * 
 * @param   list[IN] the list
 */
void dspttk_lst_deinit(DSPTTK_LIST *list);

#ifdef __cplusplus
}
#endif

#endif //_DSPTTK_POSIX_IPC_H_

