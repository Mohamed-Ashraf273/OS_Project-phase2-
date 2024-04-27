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
};
//phase2 code Extras Start------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct TNode {
    struct Process* pro;
    int isEmpty; // 1: empty & 0: full
    int size;
    struct TNode* left;
    struct TNode* right;
    
} TNode;

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
                printf("%d ", currentNode->size);
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
    // printf("myMemory: \n");
    // printTreeLevelOrder(mem);
    int sum = 0;
    int isFnd = 0;
    sumLeavesBefore(mem->root, minLeaf, &sum, &isFnd);

    minLeaf->pro->startIndex = sum;
    minLeaf->pro->endIndex = sum + minLeaf->size - 1;


    return 1;
}
void removeLeavesWithSameParent(TNode* root) {
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
        return;
    }

    // Recursively check left and right subtrees
    removeLeavesWithSameParent(root->left);
    removeLeavesWithSameParent(root->right);
}
void setSearchEmpty(TNode* root, int process_id) {
    if (root == NULL)
        return;

    // If the current node contains the process with the specified ID, set isEmpty to 1
    if (root->pro != NULL && root->pro->process_id == process_id) {
        root->isEmpty = 1;
        return;
    }

    // Recursively search the left and right subtrees
    setSearchEmpty(root->left, process_id);
    setSearchEmpty(root->right, process_id);
}

void setSearchEmptyById(memory* mem, int process_id) {
    setSearchEmpty(mem->root, process_id);
}

memory myMemory;

int main(int argc, char* argv) {
    initializeMem(&myMemory);
    struct Process p1;
    p1.memsize=59;
    struct Process p2;
    p2.memsize=146;
    struct Process p3;
    p3.memsize=33;
    struct Process p4;
    p4.memsize=61;
    struct Process p5;
    p5.memsize=32;
    addProcessToMem(&myMemory,&p1);
    addProcessToMem(&myMemory,&p2);
    addProcessToMem(&myMemory,&p3);
    addProcessToMem(&myMemory,&p4);
    addProcessToMem(&myMemory,&p5);

    printf("p1 from %d to %d\n",p1.startIndex,p1.endIndex);
    printf("p2 from %d to %d\n",p2.startIndex,p2.endIndex);
    printf("p3 from %d to %d\n",p3.startIndex,p3.endIndex);
    printf("p4 from %d to %d\n",p4.startIndex,p4.endIndex);
    printf("p5 from %d to %d\n",p5.startIndex,p5.endIndex);

    return 0;
}
