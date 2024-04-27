#include "headers.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <math.h>
typedef struct TNode {
    struct Process* pro;
    int isEmpty; // 1: empty & 0: full
    int size;
    struct TNode* left;
    struct TNode* right;
    
} TNode;
struct Process        // to store process information and send them to scheduler
{                     // long mtype;//FOR MESSAGE
    int process_id;   // process id
    int arrive_time;  // process arival time
    int running_time; // needed time to run this process
    int priority;     // priority of the process
    int memsize;
    int finish;       // finish time
    int start;        // first time it running by cpu
    int state;        // state of the process (0:running &  1:stopped or blocked & 2:term)
    int child_id;     // when fork to use it when make signals
    int remainingTime;
    int RRtime;     // store the time at when process start or resumed for RR
    bool firsttime; // first time it runs
    int startIndex;
    int endIndex;
    TNode* myBlockInMemory;
};
//phase2 code Extras Start------------------------------------------------------------------------------------------------------------------------------------------------------
FILE *memory_log;


typedef struct memory {
    TNode* root;
} memory;

void initializeMem(memory* mem) {
    mem->root = (TNode*)malloc(sizeof(TNode)); // Allocate memory for the root node
    mem->root->isEmpty = 1; // Empty
    mem->root->size = 1024;
    mem->root->pro = NULL;
    mem->root->left = mem->root->right = NULL;
}

void findMinLeaf(TNode* root, TNode** minLeaf, struct Process* pr) {
    if (root == NULL)
        return;

    // Check if the current node is a leaf, empty, and large enough to fit the process
    if (root->left == NULL && root->right == NULL && root->isEmpty == 1 && root->size >= pr->memsize) {
        if (*minLeaf == NULL || root->size < (*minLeaf)->size) {
            *minLeaf = root;
        }
        return;
    }

    // Recursively search the left and right subtrees
    findMinLeaf(root->left, minLeaf, pr);
    findMinLeaf(root->right, minLeaf, pr);
}

TNode* getMinLeaf(memory* mem, struct Process* pr) {
    TNode* minLeaf = NULL;
    findMinLeaf(mem->root, &minLeaf, pr);
    return minLeaf;
}

void sumLeavesBefore(TNode* root, TNode* target, int* sum, int* isFound) {
    if (root == NULL || root == target){
        if(root == target){
            *isFound = 1;
        }
        return;
    }

    // If it is a leaf add its size to sum
    if (root->right == NULL && root->left == NULL) {
        *sum += root->size;
        return;
    }

    // Recursively search the left and right subtrees
    sumLeavesBefore(root->left, target, sum, isFound);
    if(!(*isFound)){// if it is still not found
        sumLeavesBefore(root->right, target, sum, isFound);
    }
}

void printTreeLevelOrder(memory* mem) {
    if (mem == NULL || mem->root == NULL)
        return;

    // Queue to store nodes at each level
    TNode* queue[1000];  // Assuming a maximum of 1000 nodes
    int front = 0, rear = 0;

    queue[rear++] = mem->root;

    while (front < rear) {
        int levelSize = rear - front;  // Number of nodes at the current level

        // Process nodes at the current level
        for (int i = 0; i < levelSize; i++) {
            TNode* currentNode = queue[front++];

            // Print the process ID if the node is not empty
            if (currentNode->isEmpty == 0 && currentNode->pro != NULL) {
                printf("%d ", currentNode->pro->process_id);
            } else {
                printf("E ");  // Print 'E' for empty nodes
            }

            // Enqueue the left child
            if (currentNode->left != NULL) {
                queue[rear++] = currentNode->left;
            }

            // Enqueue the right child
            if (currentNode->right != NULL) {
                queue[rear++] = currentNode->right;
            }
        }
        printf("\n");  // Move to the next line after printing a level
    }
}
int addProcessToMem(memory* mem, struct Process* pr) {
    TNode* minLeaf = getMinLeaf(mem, pr);
    if (minLeaf == NULL) {// no empty place
        return 0;
    }
    while ((minLeaf->size >= pr->memsize) && (!((minLeaf->size) / 2 < pr->memsize))) {
        // Split
        TNode* nLeft = (TNode*)malloc(sizeof(TNode));
        TNode* nRight = (TNode*)malloc(sizeof(TNode));
        nLeft->size = minLeaf->size / 2;
        nLeft->left = NULL;
        nLeft->right = NULL;
        nLeft->isEmpty = 1;
        nRight->size = minLeaf->size / 2;
        nRight->left = NULL;
        nRight->right = NULL;
        nRight->isEmpty = 1;
        minLeaf->right = nRight;
        minLeaf->left = nLeft;
        minLeaf = nLeft;
    }
    minLeaf->isEmpty = 0;
    minLeaf->pro = pr;
    pr->myBlockInMemory = minLeaf;
    // printf("myMemory: \n");
    // printTreeLevelOrder(mem);
    int sum = 0;
    int isFnd = 0;
    sumLeavesBefore(mem->root, minLeaf, &sum, &isFnd);

    minLeaf->pro->startIndex = sum;
    minLeaf->pro->endIndex = sum + minLeaf->size - 1;
    fprintf(memory_log, "At time %d allcoated %d bytes for process %d from %d to %d \n", getClk(), pr->memsize, pr->process_id, pr->startIndex, pr->endIndex);
    return 1;
}
void removeLeavesWithSameParent(TNode* root, int* foundEmpLeaves) {
    if (root == NULL)
        return;

    // Check if both children are leaves with isEmpty = 1
    if (root->left != NULL && root->right != NULL &&
        root->left->left == NULL && root->left->right == NULL && root->left->isEmpty == 1 &&
        root->right->left == NULL && root->right->right == NULL && root->right->isEmpty == 1) {
        // Remove the leaves by setting their parent pointers to NULL
        free(root->right);
        free(root->left);
        root->left = NULL;
        root->right = NULL;
        *foundEmpLeaves = 1;
        return;
    }

    // Recursively check left and right subtrees
    removeLeavesWithSameParent(root->left, foundEmpLeaves);
    removeLeavesWithSameParent(root->right, foundEmpLeaves);
}
memory myMemory;
//phase2 code Extras End------------------------------------------------------------------------------------------------------------------------------------------------------

int Start;
struct Time
{
    int remmining;
    int start_time;
};
// Node structure
typedef struct Node
{
    struct Process *data;
    struct Node *next;
    struct Node *prev;
} Node;

// Linked list structure
typedef struct
{
    Node *head;
    Node *tail;
} LinkedList;
///  global
void handleReadyWaiting();
int algo;
void *shr;
FILE *scheduler_log;
FILE *scheduler_perf;
LinkedList Ready;
LinkedList waitingList;
int cont = 1;
int RunningTime = 0;
Node *RunningProcess = NULL;      // running process
int number_of_finish_process = 0; // number of finished process
int processorState = 0;           // 0 ideal 1 is used
float SUMWTA = 0;
float SUMWATING = 0;
float *WATArray;
int slice=0;
////////////////////////////////FILES PRINT////////////////

void Finish()
{
    printf("Finished id %d\n",RunningProcess->data->process_id);
    //phase2 start-------------
    //problem here i don't know why?##################################################

    //setSearchEmptyById(&myMemory,RunningProcess->data->process_id);//setting isEmpty of the node by 1
    
    RunningProcess->data->myBlockInMemory->isEmpty = 1;
    
    //#################################################################################
    printf("myMemoryBefore: \n");
    printTreeLevelOrder(&myMemory);
    int foundEmptyLeaves;
    do{
        foundEmptyLeaves = 0;
        removeLeavesWithSameParent(myMemory.root, &foundEmptyLeaves);// combine if possible
    }
    while (foundEmptyLeaves);
    printf("myMemoryAfter: \n");
    printTreeLevelOrder(&myMemory);
    fprintf(memory_log, "At time %d freed %d bytes for process %d from %d to %d \n", getClk(), RunningProcess->data->memsize, RunningProcess->data->process_id, RunningProcess->data->startIndex, RunningProcess->data->endIndex);
    if(waitingList.head != NULL){// check waiting list if it contains any processes
        if(addProcessToMem(&myMemory, waitingList.head->data)){//if entered
            //handleRadyAndWaiting
            handleReadyWaiting();
            printf("a process goes from wait to ready\n");
        }
    }
    //phase2 ends--------------
    int TA = getClk() - (RunningProcess->data->arrive_time);
    int wait = (RunningProcess->data->start) - (RunningProcess->data->arrive_time);
    float WTA = (float)wait / (RunningProcess->data->running_time);
    WTA = roundf(WTA * 100) / 100;
    SUMWTA += WTA;                            // sum wating trun around time
    SUMWATING += wait;                        // sum waiting time
    WATArray[number_of_finish_process] = WTA; // TO CALULATE STANDERED WAT
    fprintf(scheduler_log, "At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f \n", getClk(), RunningProcess->data->process_id, RunningProcess->data->arrive_time, RunningProcess->data->running_time, wait, TA, WTA);

    number_of_finish_process++;      // number of finished processes
    RunningProcess->data->state = 2; // term;
    RunningProcess->data->remainingTime = 0;
}
void StartorResume()
{
    if (RunningProcess != NULL)
    {
        int wait = (RunningProcess->data->start) - (RunningProcess->data->arrive_time);
        // printf("at clock= %d start =%d\n",getClk(),(RunningProcess->data->start));
        if (RunningProcess->data->firsttime)
        {
            fprintf(scheduler_log, "At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->data->process_id, RunningProcess->data->arrive_time, RunningProcess->data->running_time, RunningProcess->data->remainingTime, wait);
            RunningProcess->data->firsttime = false;
        }
        else
            fprintf(scheduler_log, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->data->process_id, RunningProcess->data->arrive_time, RunningProcess->data->running_time, RunningProcess->data->remainingTime, wait);
    }
}
void Stop()
{
    if (RunningProcess != NULL)
    {
        int wait = (RunningProcess->data->start) - (RunningProcess->data->arrive_time);

        fprintf(scheduler_log, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->data->process_id, RunningProcess->data->arrive_time, RunningProcess->data->running_time, RunningProcess->data->remainingTime, wait);
    }
}
/////////////////////////////////////////////////////
void initializeList(LinkedList *list)
{
    list->head = NULL;
    list->tail = NULL;
}
///////////Reamove functions/////////////
void RemH(LinkedList *list)
{ // remove from head
    if (list->head != NULL)
    {
        Node *current = list->head;
        list->head = list->head->next;
        if (list->head == NULL)
        {
            list->tail = list->head;
        }

        current->next = NULL;
        if (list->head != NULL)
            list->head->prev = NULL;

        free(current);
    }
}
void RemT(LinkedList *list)
{ // remove from tail
    if (list->tail != NULL)
    {
        Node *current = list->tail;
        list->tail = list->tail->prev;
        if (list->tail == NULL)
        {
            list->head = list->tail;
        }
        current->next = NULL;
        if (list->tail != NULL)
            list->tail->next = NULL;
        free(current);
    }
}
////////////////////Insertion FUNCTIONS////////////////////
void AddSorted(LinkedList *list, struct Process *data)
{ // add to head
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }
    newNode->data = data;
    if (list->head == NULL)
    { // empty
        list->head = list->tail = newNode;
        newNode->next = NULL;
        newNode->prev = NULL;
    }
    else
    {
        if (RunningProcess != NULL) // there is a process running
        {
            if (RunningProcess->data->remainingTime - 1 == 0) // it is time to be finished
            {
                ((struct Time *)shr)->remmining = RunningProcess->data->remainingTime - 1; // send it to process
                RunningProcess->data->remainingTime--;
                RunningProcess->data->state = 2; // TERM
                Finish();                        // PRINT                                       // finish remove                                                                 // to print its information
                RemH(&Ready);                    // REMOVE
                processorState = 0;              // IDEL
                RunningProcess = NULL;           // PROCESSOR IDEL

                ////////////////   add the new one
                Node *prev = NULL;
                Node *current = Ready.head;
                if (current == NULL) // LIST IS EMPTY
                {
                    list->head = list->tail = newNode;
                    newNode->next = NULL;
                    newNode->prev = NULL;
                }
                else
                {
                    while (current != NULL && current->data->remainingTime < newNode->data->remainingTime)
                    {
                        prev = current;
                        current = current->next;
                    }
                    if (current == NULL)
                    {
                        newNode->prev = Ready.tail;
                        Ready.tail->next = newNode;
                        Ready.tail = newNode;
                    }
                    else if (current == Ready.head) // must be the first one in the list
                    {
                        newNode->next = Ready.head;
                        Ready.head->prev = newNode;
                        Ready.head = newNode;
                    }
                    else
                    {
                        newNode->next = prev->next;
                        newNode->prev = prev;
                        prev->next = newNode;
                    }
                }
            }
            else if (RunningProcess->data->remainingTime - 1 > newNode->data->remainingTime) // one came is shorter than the running one
            {
                RunningProcess->data->remainingTime--;         // BECAUSE IT FINISH 1 SECOND MUST STOP
                RunningProcess->data->state = 1;               // BLOCK
                Stop();                                        // print its information
                kill(RunningProcess->data->child_id, SIGUSR2); // stop old one
                newNode->next = Ready.head;
                Ready.head->prev = newNode;
                Ready.head = newNode;
                RunningProcess = NULL;
                processorState = 0; // ideal
            }
            else // not stop the process which is running
            {
                Node *prev = Ready.head;
                Node *current = prev->next;
                while (current != NULL && current->data->remainingTime < newNode->data->remainingTime)
                {
                    prev = current;
                    current = current->next;
                }
                if (current == NULL)
                {
                    newNode->prev = Ready.tail;
                    Ready.tail->next = newNode;
                    Ready.tail = newNode;
                }
                else
                {
                    newNode->next = prev->next;
                    newNode->prev = prev;
                    prev->next = newNode;
                }
            }
        }
        else // running process is null
        {
            Node *prev = NULL;
            Node *current = Ready.head;
            while (current != NULL && current->data->remainingTime < newNode->data->remainingTime)
            {
                prev = current;
                current = current->next;
            }
            if (current == Ready.head)
            {
                newNode->next = Ready.head;
                Ready.head->prev = newNode;
                Ready.head = newNode;
            }
            else
            {
                newNode->next = prev->next;
                newNode->prev = prev;
                prev->next = newNode;
            }
        }
    }
}

void AddT(LinkedList *list, struct Process *data)
{ // add to tail
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }
    newNode->data = data;
    newNode->next = NULL;

    if (list->head == NULL)
    { // List is empty
        list->head = list->tail = newNode;
    }
    else
    {
        list->tail->next = newNode; // Link the current tail to the new node
        newNode->prev = list->tail; // Update the previous pointer of the new node
        list->tail = newNode;       // Update the tail pointer to the new node
    }
}

void AddSortedPriority(LinkedList *list, struct Process *data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }
    newNode->data = data;
    newNode->next = NULL;
    newNode->prev = NULL; // Initialize prev pointer

    if (list->head == NULL)
    { // Empty list
        list->head = list->tail = newNode;
    }
    else
    {
        Node *current = list->head;
        if (list->head->data->arrive_time != data->arrive_time) // not came at same time
        {
            while (current->next != NULL && data->priority >= current->next->data->priority) // first one is running
            {
                current = current->next;
            }
            if (current->next == NULL)
            { // Insert at the end
                current->next = newNode;
                newNode->prev = current;
                list->tail = newNode; // Update tail
            }
            else
            { // Insert in the middle
                newNode->next = current->next;
                current->next->prev = newNode;
                current->next = newNode;
                newNode->prev = current;
            }
        }
        else
        {
            while (current != NULL && data->priority >= current->data->priority) // came at same time
            {

                current = current->next;
            }
            if (current == NULL)
            { // Insert at the end
                current = list->tail;
                current->next = newNode;
                newNode->prev = current;
                list->tail = newNode; // Update tail
            }
            else
            { // Insert in the middle
                if (current == list->head)
                {
                    newNode->next = current;
                    current->prev = newNode;
                    newNode->prev = NULL;
                    list->head = newNode;
                }
                else
                {
                    newNode->next = current;
                    newNode->prev = current->prev;
                    current->prev->next = newNode;
                    newNode->prev = newNode;
                }
            }
        }
    }
}
/// HPF
///
void SortAccordingRT(LinkedList *list)
{
    if (list->head == NULL || list->head == list->tail)
    {
        return; // Nothing to sort
    }

    Node *current1 = list->head;
    while (current1 != NULL)
    {
        Node *current2 = current1->next;
        Node *minNode = current1; // Assume current node has minimum remaining time

        // Find the node with minimum remaining time starting from current2
        while (current2 != NULL)
        {
            if (current2->data->remainingTime < minNode->data->remainingTime)
            {
                minNode = current2;
            }
            current2 = current2->next;
        }

        // Swap data between current1 and minNode
        struct Process *temp = current1->data;
        current1->data = minNode->data;
        minNode->data = temp;

        current1 = current1->next; // Move to next node
    }
}
void handleReadyWaiting(){
    switch (algo)//add it to ready
    {
        case 1:
            AddT(&Ready,waitingList.head->data);
            break;
        case 2:
            AddSorted(&Ready,waitingList.head->data);
            break;
        case 3:
            AddSortedPriority(&Ready,waitingList.head->data);
            break;
        default:
            printf("Invalid algorithm!\n");
            break;
    }
    RemH(&waitingList);
}

/////////////////////////////////////////////////////////
void printList(LinkedList *list)
{
    if (list->head != NULL)
    {
        Node *current = list->head;
        while (current != NULL)
        {
            printf("%d, ", current->data->remainingTime);
            current = current->next;
        }
    }
}
// 0:idle & 1:working on process
struct statisticsP
{
    int RunningTime;
    int TA;
};
struct statisticsP *result; // I will need
////ALGO FUNCTION
void ALGO(LinkedList *list)
{
   //printf("hello clk %d\n", getClk());
    if (list->head != NULL)
    {
        if (RunningProcess == NULL && list->head != NULL)
            processorState = 0;
        if (!processorState) // ideal
        {                    // if idle
                             // printf("idel\n");
            RunningProcess = list->head;
            RunningProcess->data->state = 0; // running
            processorState = 1;              // busy
            ((struct Time *)shr)->remmining = RunningProcess->data->remainingTime;
            kill(RunningProcess->data->child_id, SIGCONT);
            // calc for start or resumed process
            if (RunningProcess->data->running_time == RunningProcess->data->remainingTime)
            {
                RunningProcess->data->start = getClk();
                RunningProcess->data->firsttime = true;
            }
            StartorResume();
        }
        else
        {
            if (RunningProcess != NULL)
            {
                RunningProcess->data->remainingTime--;
                ((struct Time *)shr)->remmining = RunningProcess->data->remainingTime;
                //printf("process with time = %d at clk =%d remining =%d\n", RunningProcess->data->running_time, getClk(), RunningProcess->data->remainingTime);
                if (RunningProcess->data->remainingTime == 0)
                { // terminated
                    Finish();

                    RemH(list); // remove it from list

                    RunningProcess = list->head;
                    if (RunningProcess != NULL)
                    {
                        RunningProcess->data->state = 1; // blocked
                        processorState = 0;              // idle
                        kill(RunningProcess->data->child_id, SIGCONT);
                        RunningProcess->data->state = 0; // running
                        processorState = 1;              // busy
                        if (RunningProcess->data->running_time == RunningProcess->data->remainingTime)
                        {
                            RunningProcess->data->firsttime = true;

                            RunningProcess->data->start = getClk();
                        }
                        // calc for start or resumed process
                        StartorResume();
                    }
                }
            }
        }
    }
}
void RRSwitch(LinkedList *Ready)
{
    if (Ready->head != NULL)
    {
        if (RunningProcess != NULL)
        {
            if (RunningProcess->data->remainingTime - 1 == 0)
            {
                //printf("remove%d\n", getClk());
                RunningProcess->data->remainingTime--;
                RunningProcess->data->state = 2; // TERMINATE
                Finish();                        // finish
                processorState = 0;              // ideal
                RunningProcess = NULL;
                RemH(Ready); // REMOVE IT
            }
            else if (((RunningProcess->data->running_time) - (RunningProcess->data->remainingTime - 1)) % slice == 0)
            {
                RunningProcess->data->remainingTime--;
                RunningProcess->data->state = 1; // stop
                Stop();
               // printf("switch %d\n", getClk());

                AddT(Ready, (Ready->head->data)); // return to add it in the tail of list
                RemH(Ready);                      // remove it from head
                processorState = 0;
                RunningProcess = NULL;
                //            printList(&Ready);
            }
        }
        ALGO(Ready);
    }
    else
    {
        processorState = 0; // Idle
    }
}
/////////////////////SIGNAL FUNCTIONS
void handleChild(int signum) // this is when a process send this to scheduler
{

    Start = ((struct Time *)shr)->start_time;
    if (Start == 0) // process first created
    {
        cont = 0;
    }

    signal(SIGUSR1, handleChild);
}
void ClearCock(int signum) // it process_generator interrupt;
{
    shmdt(shr);

    destroyClk(false); // release lock

    exit(0); // exit
}
////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    initializeList(&Ready);
    initializeList(&waitingList);
    initializeMem(&myMemory);
    Node *RRPointer = Ready.head; // pointer points to the first process for RR Algo
    signal(SIGUSR1, handleChild); // if child terminate process
    signal(SIGINT, ClearCock);
    int msg_id = atoi(argv[1]);    // ID OF MESSAGE QUEUE
    int shm_id = atoi(argv[2]);    // ID OF Shared memory
    int algorithm = atoi(argv[3]); // 1 RR 2 SRTN 3 HPF //Mohammed use this
    algo = algorithm;
    slice = atoi(argv[4]);
    int number_of_system_process = atoi(argv[5]); // all system processes
    char process_run_time[10];                    // to send it to each process
    char share_id[10];                            // to send it to each process
    sprintf(share_id, "%d", shm_id);
    initClk();
    int i = 0;
    int prev = -1;
    WATArray = malloc(sizeof(float) * number_of_system_process);
    scheduler_log = fopen("scheduler.log", "w");
    fprintf(scheduler_log, "#At time x process y state arr w total z remain y wait k\n");
    scheduler_perf = fopen("scheduler.perf", "w");
    memory_log = fopen("memory.log", "w");
    fprintf(memory_log, "#At time x allcoated y bytes for process z from i to j\n");
    int num = 0;
    int busytime;
    struct Process *process = (struct Process *)malloc(number_of_system_process * sizeof(struct Process));
    while (number_of_finish_process < number_of_system_process) // untill finish
    {

        int size = 20;
        if (prev != getClk()) // 0 1 check prev run next
        {
            while (num != number_of_system_process && msgrcv(msg_id, &process[num], size, 0, IPC_NOWAIT) != -1) // recieve a process                                                                                                               // new process I think need to e scheduled                                                                                                          // receive a process
            {
                //printf("process[%d] rec with size: %d\n",num, process[num].memsize);
                sprintf(process_run_time, "%d", process[num].running_time);
                busytime += process[num].running_time;
                if (process[num].arrive_time > getClk()) // if process send process
                {
                //    printf("wait process which time is %d come at %d must be at %d\n", process[num].running_time, getClk(), process[num].arrive_time);
                    ;
                    if (algorithm == 1)
                        RRSwitch(&Ready);
                    else
                        ALGO(&Ready); // continue but dosent but the newest one
                    while (process[num].arrive_time > getClk())
                        ; // wait
                }
               // printf("clockrecieve: %d\n", getClk());

                int child_id = fork();
                // stop
                if (child_id == -1)
                {
                    printf("Error when try to fork \n");
                }
                else if (child_id == 0) // child code
                {
                    execl("process", "process", share_id, process_run_time, NULL);
                }
                else // scheduler
                {
                    process[num].child_id = child_id; // set process id

                    if (i == 0) // for first time only
                    {
                        shr = shmat(shm_id, NULL, 0);
                        i++;
                    }
                    process[num].remainingTime = process[num].running_time;
                    process[num].state = 1;
                    process[num].firsttime = false;

                    switch (algorithm)
                    {
                    case 1: // Round Robin
                        if(addProcessToMem(&myMemory,&process[num])){//try but to memory else put in waiting list
                            AddT(&Ready, &process[num]);
                            // printf("ReadyList: \n");
                            // printList(&Ready);
                            
                        }else{
                            AddT(&waitingList, &process[num]);
                            // printf("WaitingList: \n");
                            // printList(&waitingList);
                        }
                        
                        num++;
                        break;
                    case 2:
                        if(addProcessToMem(&myMemory,&process[num])){//try but to memory else put in waiting list
                            AddSorted(&Ready, &process[num]); // new one came
                            // printf("ReadyList: \n");
                            // printList(&Ready);
                        }else{
                            AddT(&waitingList, &process[num]);

                            // printf("WaitingList: \n");
                            // printList(&waitingList);
                        }
                        //          printList(&Ready);
                        //     printf("clockbefore %d prev %d\n", getClk(), prev);

                        num++;
                        break;
                    case 3: // Highest Priority First (HPF) Sorted
                        // printf("hello from HPF\n");
                        if(addProcessToMem(&myMemory,&process[num])){//try but to memory else put in waiting list
                            AddSortedPriority(&Ready, &process[num]); // new one came
                            // printf("ReadyList: \n");
                            // printList(&Ready);
                            
                        }else{
                            AddT(&waitingList, &process[num]);
                            // printf("WaitingList: \n");
                            // printList(&waitingList);
                        }
                        // printf("\n");
                        
                        // printf("processEntered from %d to %d\n",process[num].startIndex,process[num].endIndex);
                        num++;

                        break;
                    default:
                        printf("Invalid algorithm!\n");
                        break;
                    }
                    // printf("myMemoryFirstCommingEntering: \n");
                    // printTreeLevelOrder(&myMemory);
                    // printf("\n");
                    // wait(NULL);
                    // printf("clock1: %d\n",getClk());
                    // while (cont)
                    //{
                    //}
                    // printf("clock1: %d\n",getClk());
                    // cont = 1;
                    raise(SIGSTOP);
                }
                //  prev = getClk() - 1; // to enter algorithm
            }

            // printf("clock%d\n", getClk());
            switch (algorithm)
            {
            case 1: // Round Robin
                RRSwitch(&Ready);
                break;

            case 2:

                if (Ready.head != NULL)
                {

                    // SortAccordingRT(&Ready);
                    ALGO(&Ready);
                }
                else
                {
                    processorState = 0; // idle
                }
                break;
            case 3: // Highest Priority First (HPF)
                if (Ready.head != NULL)
                {
                    ALGO(&Ready);
                }
                else
                {
                    processorState = 0; // idle
                                        //  ALGO(&Ready);
                }
                break;
            default:
                printf("Invalid algorithm!\n");
                break;
            }

            prev = getClk();
        }
    }

    /**
     calculation of second output file must be here
    */
    float CPUuti = 1.0 * busytime / getClk();          // cpu
    float AVGWAT = SUMWTA / number_of_system_process;  // avrage waited turn around time
    float AVGW = SUMWATING / number_of_system_process; // avrage waited
    AVGWAT = roundf(AVGWAT * 100) / 100;
    AVGW = roundf(AVGW * 100) / 100;
    float STDWAT = 0;
    for (int i = 0; i < number_of_system_process; i++)
    {
        STDWAT += pow((WATArray[i] - AVGWAT), 2);
    }
    STDWAT /= number_of_system_process;
    STDWAT = sqrt(STDWAT);
    STDWAT = round(STDWAT * 100) / 100;
    CPUuti = roundf(CPUuti * 10000) / 100;
    fprintf(scheduler_perf, "CPU utilization = %.2f\n", CPUuti);
    fprintf(scheduler_perf, "AWG WAT= %.2f\n", AVGWAT);
    fprintf(scheduler_perf, "AVG Waiting = %.2f\n", AVGW);
    fprintf(scheduler_perf, "STD WAT = %.2f\n", STDWAT);

    //////////////////////////////////CLEAR
    free(WATArray);
    shmdt(shr);
    fclose(scheduler_log);
    fclose(scheduler_perf);
    fclose(memory_log);
    destroyClk(false);
    kill(getppid(), SIGINT); // IF scheduler terminate mean all process he fork also terminate so must terminate process_generator
    exit(0);
}