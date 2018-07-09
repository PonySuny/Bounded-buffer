#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>
#include<math.h>
typedef  struct{
    int buffer[10];
    int head;
    int tail;
}Input,Output;

Input *BufferIn;
Output *BufferOut;

int operator_num = 3;
int tool_num = 3;
int wait_num = 0;
int deadlock_num = 0;
int MaterialNumberNow[4] = {0, 0, 0, 0};
int MaterialNumberUsed[4] = {0, 0, 0, 0};

int output_buffer[3] = {0, 0, 0};
int input_buffer[3] = {0, 0, 0};
int test_produce[3] = {1, 1, 1};

int PauseSig = 0;

pthread_mutex_t waitmu, inputmu, ouputmu;
pthread_cond_t cdt1, cdt2, cdt3;
pthread_t opre[20];

int check_repetition(int x)
{
    if (x == input_buffer[0])
        return 0;
    return 1;
}

void check_diff()
{
    if (abs(output_buffer[0] - output_buffer[1]) > 10)
        if (output_buffer[0] > output_buffer[1])
            test_produce[0] = 0;
    if (output_buffer[0] < output_buffer[1])
        test_produce[1] = 0;
    if (abs(output_buffer[0] - output_buffer[2]) > 10)
        if (output_buffer[0] > output_buffer[2])
            test_produce[0] = 0;
    if (output_buffer[0] < output_buffer[2])
        test_produce[2] = 0;
    if (abs(output_buffer[1] - output_buffer[2]) > 10)
        if (output_buffer[1] > output_buffer[2])
            test_produce[1] = 0;
    if (output_buffer[1] < output_buffer[2])
        test_produce[2] = 0;
}

int read_input(Input *p)
{
    int x = p -> buffer[p -> tail];
    p -> tail ++;
    if (p -> tail == 10)
        p -> tail = 1;
    return x;
}

void insert_input(Input *p, int x)
{
    p -> buffer[p -> head] = x;
    p -> head += 1;
    input_buffer[2] = input_buffer[1];
    input_buffer[1] = input_buffer[0];
    input_buffer[0] = x;
    if(p -> head == 10)
        p -> head = 0;
}

void insert_output(Output *p, int x)
{
    if (p -> head - p -> tail == 9)
        p -> tail = 1;
    if (p -> tail - p -> head == 1){
        p -> tail ++;
        if (p -> tail == 10)
            p -> tail = 1;
    }
    insert_input(p, x);
}

void pproduct(int x, int y)
{
    test_produce[0] = 1;
    test_produce[1] = 1;
    test_produce[2] = 1;
    if (abs(x-y) == 2){
        insert_output(BufferOut, 2);
        output_buffer[1]++;
        test_produce[1] = 0;
    }
    if (abs(x-y) == 1){
        if((x+y) == 3){
            insert_output(BufferOut, 1);
            output_buffer[0]++;
            test_produce[0] = 0;
        }
    }
    else{
        insert_output(BufferOut, 3);
        output_buffer[2]++;
        test_produce[2] = 0;
    }
    check_diff();
}

int check_produce(int x, int y)
{
    if (abs(x-y) == 2)
        return test_produce[1];
    if (abs(x-y) == 1)
        if((x+y) == 3)
            return test_produce[0];
        if((x+y) == 5)
            return test_produce[2];
}

int check_buffin(Input *p)
{
    if (p -> head == p -> tail)
        return 0;
    if (p -> head - p -> tail == 9 || p -> tail - p -> head == 1)
        return -1;
    return 1;
}

void * producer1()
{
    while(1)
    {
        pthread_mutex_lock(&inputmu);
        while(check_buffin(BufferIn) || !check_repetition(1)){
                pthread_cond_signal(&cdt3);
                pthread_cond_wait(&cdt3, &inputmu);
        }
        insert_input(BufferIn, 1);
        MaterialNumberNow[1]++;
        MaterialNumberUsed[1]++;
        pthread_cond_signal(&cdt3);
        pthread_mutex_unlock(&inputmu);
        while(PauseSig)
            usleep(100);
    }
    pthread_exit(0);
}

void * producer2()
{
    while(1)
    {
        pthread_mutex_lock(&inputmu);
        while(check_buffin(BufferIn) || !check_repetition(2)){
                pthread_cond_signal(&cdt3);
                pthread_cond_wait(&cdt3, &inputmu);
        }
        insert_input(BufferIn, 2);
        MaterialNumberNow[2]++;
        MaterialNumberUsed[2]++;
        pthread_cond_signal(&cdt3);
        pthread_mutex_unlock(&inputmu);
        while(PauseSig)
            usleep(100);
    }
    pthread_exit(0);
}

void * producer3()
{
    while(1)
    {
        pthread_mutex_lock(&inputmu);
        while(check_buffin(BufferIn) || !check_repetition(3)){
                pthread_cond_signal(&cdt3);
                pthread_cond_wait(&cdt3, &inputmu);
        }
        insert_input(BufferIn, 3);
        MaterialNumberNow[3]++;
        MaterialNumberUsed[3]++;
        pthread_cond_signal(&cdt3);
        pthread_mutex_unlock(&inputmu);
        while(PauseSig)
            usleep(100);
    }
    pthread_exit(0);
}

void * operators()
{
    int x, y;
    while(1)
    {
        while(tool_num < 2)
            pthread_cond_wait(&cdt1, &waitmu);
        tool_num -= 2;
        pthread_mutex_lock(&inputmu);
        x = read_input(BufferIn);
        pthread_mutex_lock(&ouputmu);
        pthread_mutex_unlock(&inputmu);
        check_repetition(x);
        usleep(1);
        tool_num += 2;
        y = read_input(BufferIn);
        if(!check_produce(x,y))
            wait_num++;
        if(wait_num >= operator_num){
            wait_num -= 1;
            deadlock_num +=1;
        }
        pproduct(x, y);
        pthread_mutex_unlock(&ouputmu);
        wait_num--;
        while (PauseSig)
            usleep(100);
    }
    pthread_exit(0);
}

void NumTO(){
    int i = 0;
    printf("Enter the tool number(3-30, other number will be set to default 3): ");
    scanf("%d", &i);
    tool_num = (i>=3&&i<=30)?i:3;
    printf("Enter the Operator number(3-10, other number will be set to default 3): ");
    scanf("%d", &i);
    operator_num = (i>=3&&i<=10)?i:3;
}

void * dynamic_output()
{
    int i;
    while(1){
        while(PauseSig)
            usleep(100);
        printf("Material 1: %d, Material 2: %d, Material 3: %d\n", MaterialNumberNow[1], MaterialNumberNow[2], MaterialNumberNow[3]);
        printf("Input Buffer: ");
        i = BufferOut -> tail;
        while(i != BufferIn -> head)
        {
            i--;
            if(i == -1)
                i = 9;
            printf("%d ", BufferIn -> buffer[i]);
        }
        printf("\nProduct A(Tool x & y): %d, Pruduct B(Tool y & z): %d, Product C(Tool x & z): %d \n", output_buffer[0], output_buffer[1], output_buffer[2]);
        printf("Output Buffer: ");
        i = BufferOut -> head;
        while(i != BufferOut -> tail)
        {
            i --;
            if(i == -1)
                i = 9;
            printf("%d ", BufferOut -> buffer[i]);
        }
        printf("\nAvilable Tools: %d\n", tool_num);
        printf("Waiting process: %d\n", wait_num);
        printf("Deadlocks happened: %d\n",deadlock_num);
        printf("\n\nPress 'CTRL + Z' to pause the program\n\n\n");
        sleep(2);
    }
    pthread_exit(0);
}

void run_thread()
{
    pthread_t Produce1, Produce2, Produce3, dynamicoutput;
    int i;
    pthread_create(&dynamicoutput,NULL,dynamic_output,NULL);
    pthread_create(&Produce1,NULL,producer1,NULL);
    pthread_create(&Produce2,NULL,producer2,NULL);
    pthread_create(&Produce3,NULL,producer3,NULL);
    for (i=0;i<=operator_num;i++)
        pthread_create(&opre[i],NULL,operators,NULL);
    pthread_join(Produce1,NULL);
    pthread_join(Produce2,NULL);
    pthread_join(Produce3,NULL);
    pthread_join(dynamicoutput,NULL);
    for (i=0;i<=operator_num;i++)
        pthread_join(opre[i],NULL);
}

void Pthreadinitial()
{
    pthread_mutex_init(&inputmu,NULL);
    pthread_mutex_init(&ouputmu,NULL);
    pthread_mutex_init(&waitmu,NULL);
    pthread_cond_init(&cdt1,NULL);
    pthread_cond_init(&cdt2,NULL);
    pthread_cond_init(&cdt3,NULL);
}

void pause_handler(int s)
{
    if(s==SIGTSTP)
    {
        PauseSig=1-PauseSig;
        if(PauseSig==1){
            printf("\nProgram paused.\nTo resume, Press \'CTRL + Z\'\n") ;
        }
    }
}


int main()
{
    Pthreadinitial();
    BufferOut = (Output *)malloc(sizeof(Output));
    BufferOut -> tail = 0;
    BufferOut -> head = 0;
    BufferIn = (Input *)malloc(sizeof(Input));
    BufferIn -> tail = 0;
    BufferIn -> head = 0;
    signal(SIGTSTP, pause_handler);
    NumTO();
    run_thread();

}
