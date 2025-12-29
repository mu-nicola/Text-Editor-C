#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define INSERT 1
#define DELETE 2
#define MAX_UNDO 5
#define WORD_LEN 50

/* ================= DATA STRUCTURES ================= */

typedef struct WordNode {
    char word[WORD_LEN];
    struct WordNode *next;
} WordNode;

typedef struct Action {
    int type;                  // INSERT or DELETE
    char word[WORD_LEN];       // affected word
    int position;              // position of action
} Action;

typedef struct StackNode {
    Action act;
    struct StackNode *next;
} StackNode;

/* ================= GLOBAL STATE ================= */

WordNode *textHead = NULL;
StackNode *undoStack = NULL;
StackNode *redoStack = NULL;
int undoCount = 0;

/* ================= UTILITY FUNCTIONS ================= */

int wordCount() {
    int count = 0;
    WordNode *t = textHead;
    while (t) {
        count++;
        t = t->next;
    }
    return count;
}

void clearStack(StackNode **top) {
    while (*top) {
        StackNode *temp = *top;
        *top = temp->next;
        free(temp);
    }
}

void clearText() {
    while (textHead) {
        WordNode *temp = textHead;
        textHead = textHead->next;
        free(temp);
    }
}

/* ================= STACK OPERATIONS ================= */

void push(StackNode **top, Action a, int *count) {
    /* Enforce undo limit */
    if (*count == MAX_UNDO) {
        StackNode *cur = *top, *prev = NULL;
        while (cur->next) {
            prev = cur;
            cur = cur->next;
        }
        if (prev)
            prev->next = NULL;
        free(cur);
        (*count)--;
    }

    StackNode *n = (StackNode *)malloc(sizeof(StackNode));
    n->act = a;
    n->next = *top;
    *top = n;
    (*count)++;
}

int pop(StackNode **top, Action *a, int *count) {
    if (!*top)
        return 0;

    StackNode *temp = *top;
    *a = temp->act;
    *top = temp->next;
    free(temp);
    (*count)--;
    return 1;
}

/* ================= TEXT OPERATIONS ================= */

void insertWordAt(char *word, int pos) {
    WordNode *n = (WordNode *)malloc(sizeof(WordNode));
    strcpy(n->word, word);
    n->next = NULL;

    if (!textHead || pos <= 1) {
        n->next = textHead;
        textHead = n;
        return;
    }

    WordNode *t = textHead;
    for (int i = 1; i < pos - 1 && t->next; i++)
        t = t->next;

    n->next = t->next;
    t->next = n;
}

int deleteWordAt(int pos, char *deleted) {
    if (!textHead)
        return 0;

    WordNode *t = textHead;

    if (pos == 1) {
        strcpy(deleted, t->word);
        textHead = t->next;
        free(t);
        return 1;
    }

    for (int i = 1; i < pos - 1 && t->next; i++)
        t = t->next;

    if (!t->next)
        return 0;

    WordNode *d = t->next;
    strcpy(deleted, d->word);
    t->next = d->next;
    free(d);
    return 1;
}

void displayText() {
    printf("\n----------------------------------\n");
    printf("DOCUMENT:\n");

    WordNode *t = textHead;
    while (t) {
        printf("%s ", t->word);
        t = t->next;
    }

    printf("\n----------------------------------\n");
    printf("WORDS: %d | UNDO: %d/%d\n",
           wordCount(), undoCount, MAX_UNDO);
}

/* ================= UNDO / REDO ================= */

void undo() {
    Action a;
    if (!pop(&undoStack, &a, &undoCount)) {
        printf("\nNothing to undo.\n");
        return;
    }

    if (a.type == INSERT) {
        char temp[WORD_LEN];
        deleteWordAt(a.position, temp);
    } else {
        insertWordAt(a.word, a.position);
    }

    push(&redoStack, a, &(int){0});
}

void redo() {
    Action a;
    if (!pop(&redoStack, &a, &(int){0})) {
        printf("\nNothing to redo.\n");
        return;
    }

    if (a.type == INSERT) {
        insertWordAt(a.word, a.position);
    } else {
        char temp[WORD_LEN];
        deleteWordAt(a.position, temp);
    }

    push(&undoStack, a, &undoCount);
}

/* ================= FILE OPERATIONS ================= */

void saveToFile() {
    char fname[50];
    printf("\nEnter file name to save: ");
    scanf("%s", fname);

    FILE *fp = fopen(fname, "w");
    if (!fp) {
        printf("Cannot save file.\n");
        return;
    }

    WordNode *t = textHead;
    while (t) {
        fprintf(fp, "%s ", t->word);
        t = t->next;
    }

    fclose(fp);
    printf("File saved successfully.\n");
}

void loadFromFile() {
    char fname[50], w[WORD_LEN];
    printf("\nEnter file name to load: ");
    scanf("%s", fname);

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        printf("Cannot open file.\n");
        return;
    }

    clearText();
    clearStack(&undoStack);
    clearStack(&redoStack);
    undoCount = 0;

    while (fscanf(fp, "%s", w) != EOF)
        insertWordAt(w, wordCount() + 1);

    fclose(fp);
    printf("File loaded. Undo history cleared.\n");
}

/* ================= MAIN ================= */

int main() {
    char line[400];
    char *token;

    printf("========================================\n");
    printf("   MINI WORD PROCESSOR (C - Console)\n");
    printf("   Ctrl+Z Undo | Ctrl+Y Redo | ESC Exit\n");
    printf("========================================\n");

    printf("\nPaste or type text, then press ENTER:\n");
    getchar();
    fgets(line, sizeof(line), stdin);

    token = strtok(line, " \n");
    while (token) {
        insertWordAt(token, wordCount() + 1);

        Action a;
        a.type = INSERT;
        strcpy(a.word, token);
        a.position = wordCount();
        push(&undoStack, a, &undoCount);

        clearStack(&redoStack);
        token = strtok(NULL, " \n");
    }

    displayText();

    while (1) {
        int ch = getchar();

        if (ch == 26) {          // Ctrl+Z
            undo();
            displayText();
        } 
        else if (ch == 25) {     // Ctrl+Y
            redo();
            displayText();
        } 
        else if (ch == 27) {     // ESC
            printf("\nExiting editor.\n");
            break;
        } 
        else if (ch == 's') {
            saveToFile();
        } 
        else if (ch == 'l') {
            loadFromFile();
            displayText();
        }
    }

    clearText();
    clearStack(&undoStack);
    clearStack(&redoStack);
    return 0;
}
