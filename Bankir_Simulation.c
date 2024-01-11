#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

int resource_num = 0, process_num = 0;
pthread_mutex_t mutex;

// data:

// Вектор доступных ресурсов
int *availResourceVector;

// Матрица используемых процессами ресурсов
int **allocMatrix;

// Матрица максимального потребления для каждого ресурса
int **maxMatrix;

// Разница между максимальным потреблением и текущим
int **needMatrix;

// Функция, которая выделяет ресурсы (requestVector) потоку
int requestResource(int processID, int requestVector[]);

// Функция, освобождающая ресурсы (requestVector) потока
int releaseResource(int processID, int releaseVector[]);

// Функция для проверки запрошенных ресурсов (requestVector), превосходят ли они
// остаточные ресурсы потока

int ifGreaterThanNeed(int processID, int requestVector[]);

// Функция, которая проверяет, превосходят ли освобождаемые ресурсы (requestVector)
// используемые ресурсы потока
int ifGreaterThanRelease(int processId, int releaseVector[]);

// Функция для проверки запрошенных ресурсов (requestVector), превосходят ли они
// доступные ресурсы системы
int ifGreaterThanAlloc(int []);

// Фукция, которая оцениает текущее состояние системы
// Возвращает true, если система безопасна, иначе false
bool isSafeMode();

// Фукнция для вывода состояния системы
// (allocMatrix, maxMatrix, needMatrix)
void printState();

// Функция для вывода матрицы
void printMatrix(int **matrix);

// Функция для вывода нужного вектора ресурсов
void printVector(int *vector);

// Функция для симуляции запросов потоков
void *customer(void* customerID);

// Функция для чтения начальных данных из файла с именем filename
int readMatrix(char *filname);

int main(){
    readMatrix("data.txt");

    for (int y = 0; y < process_num; ++y){
        for (int x = 0; x < resource_num; ++x){
            needMatrix[y][x] = maxMatrix[y][x] - allocMatrix[y][x];
        }
    }
    printf("Всего ресурсов в системе:\n");
    printVector(availResourceVector);
    printf("Начальное распределение ресурсов:\n");
    printMatrix(allocMatrix);
    printf("Необходимость в ресурсах для каждого процесса:\n");
    printMatrix(needMatrix);

    //Объявляем потоки
    pthread_mutex_init(&mutex,NULL);
    pthread_attr_t attrDefault;
    pthread_attr_init(&attrDefault);
    pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t) * process_num);
    int *pid = (int*) malloc(sizeof(int)*process_num); //customer's ID

    //Создаем потоки
    for(int i = 0; i < process_num; i++){
        *(pid + i) = i;
        pthread_create((tid+i), &attrDefault, customer, (pid+i));
    }

    //join threads
    for(int i = 0; i < process_num; i++){
        pthread_join(*(tid+i), NULL);
    }
    return 0;
}

void *customer(void* customerID){
    int processID = *(int*)customerID;
    int counter = 2;
    while(counter--){
        sleep(1);
        int requestVector[resource_num];
        pthread_mutex_lock(&mutex);

        // заполняем запрос случайными значениями от 0 до значений оставшейся потребности потока
        for(int i = 0; i < resource_num; i++){
            if(needMatrix[processID][i] != 0){
                requestVector[i] = rand() % needMatrix[processID][i];
            }
            else{
                requestVector[i] = 0;
            }
        }
        printf("\n\n(!) Поток (%d) запрашивает ресурсы:\n",processID);
        printVector(requestVector);

        // пробуем выполнить запрос
        requestResource(processID, requestVector);
        pthread_mutex_unlock(&mutex);

        sleep(1);

        // вектор с количеством ресурсов, которые нужно высвободить
        int releaseVector[resource_num];

        pthread_mutex_lock(&mutex);

        // инициализируем случайно
        for(int i = 0; i < resource_num; i++){
            if(allocMatrix[processID][i] != 0){
                releaseVector[i] = rand() % allocMatrix[processID][i];
            }
            else{
                releaseVector[i] = 0;
            }
        }
        printf("\n\n(!) Поток (%d) освобождает ресурсы:\n", processID);
        printVector(requestVector);
        releaseResource(processID,releaseVector);

        pthread_mutex_unlock(&mutex);
    }
}

int requestResource(int processID, int requestVector[]){

    if (ifGreaterThanNeed(processID, requestVector) == -1){
        printf("Запрашиваемые ресурсы превосходят остаточные!\n");
        return -1;
    }
    printf("-> Распределяем...\n");

    if(ifGreaterThanAlloc(requestVector) == -1){
        printf("Запрашиваемые ресурсы превосходят доступные в системе!\n");
        return -1;
    }

    // Выделяем ресурсы
    for (int i = 0; i < resource_num; ++i){
        needMatrix[processID][i] -= requestVector[i];
        allocMatrix[processID][i] += requestVector[i];
        availResourceVector[i] -= requestVector[i];
    }

    // Проверяем статус состояния после выделения
    if (isSafeMode()){
        printf("-> Состояние безопасно.\n");
        printState();
        return 0;
    }
    else{
        printf("-> Состояние небезопасно. Возврат\n");
        // Возвращаем ресурсы системе
        for (int i = 0; i < resource_num; ++i){
            needMatrix[processID][i] += requestVector[i];
            allocMatrix[processID][i] -= requestVector[i];
            availResourceVector[i] += requestVector[i];
        }
        return -1;
    }
}

int releaseResource(int processID,int releaseVector[]){
    if(ifGreaterThanRelease(processID,releaseVector) == -1){
        printf("Освобождаемых ресурсов больше чем используемых!\n");
        return -1;
    }

    for(int i = 0; i < resource_num; i++){
        allocMatrix[processID][i] -= releaseVector[i];
        needMatrix[processID][i] += releaseVector[i];
        availResourceVector[i] += releaseVector[i];
    }

    printf("-> Освобождение успешно.\n");
    printState();
    return 0;
}

int ifGreaterThanRelease(int processID,int releaseVector[]){
    for (int i = 0; i < resource_num; ++i){				 
        if (releaseVector[i] > allocMatrix[processID][i])
            return -1;
    }
    return 0;
}

int ifGreaterThanNeed(int processID,int requestVector[]){

    for (int i = 0; i < resource_num; ++i){
        if (requestVector[i] > needMatrix[processID][i])
            return -1;
    }
    return 0;
}

int ifGreaterThanAlloc(int requestVector[]){//check if there are enough resources to allocate
    for (int i = 0; i < resource_num; ++i){
        if(requestVector[i] > availResourceVector[i])
            return -1;
    }
    return 0;
}

void printMatrix(int **matrix){
    for(int y = 0; y < process_num; y++) {
        printf("(%d) : [ ", y + 1);
        for (int x = 0; x < resource_num - 1; x++) {
            printf("%d, ", matrix[y][x]);
        }
        printf("%d ]\n", matrix[y][resource_num - 1]);
    }
}

void printVector(int *vector)
{
    int i;
    for (i = 0; i < resource_num - 1; ++i){
        printf("%d, ", vector[i]);
    }
    printf("%d\n", vector[i]);
}

bool isSafeMode(){
    int i;
    int *ifFinish = malloc(process_num * sizeof(int));
    for(i = 0; i < process_num; i++)
        ifFinish[i] = 0;

    int work[resource_num];
    for(i = 0; i < resource_num; i++){
        work[i] = availResourceVector[i];
    }
    for(int p_index = 0; p_index < process_num; p_index++){
        if (ifFinish[p_index] == 0){
            for(int res_index = 0; res_index < resource_num; res_index++){
                if(needMatrix[p_index][res_index] <= work[res_index]){
                    if(res_index == resource_num - 1){
                        ifFinish[p_index] = 1;                   // завершаем процесс
                        for (i = 0; i < resource_num; i++){
                            work[i] += allocMatrix[p_index][i]; // освобождаем все ресурсы процесса
                        }
                        p_index = -1;
                        break;
                    }
                } else{
                    break;
                }
            }
        }
    }
    for(i = 0; i < process_num; i++){
        if (ifFinish[i] == 0){
            return false;   // если хотя бы один не завершен
        }
    }
    return true;
}

void printState(){
    printf("\n[INFO] Доступные ресурсы:\n");
    printVector(availResourceVector);
    printf("\n[INFO] Текущее распределение:\n");
    printMatrix(allocMatrix);
    printf("\n[INFO] Остаточное распределение:\n");
    printMatrix(needMatrix);
}

int readMatrix(char *filname) {
    FILE *in;
    in = fopen(filname, "r");

    if (NULL == in) {
        printf("file can't be opened \n");
    }

    char temp[10];
    fgets(temp, 10, in);          // считываем число ресурсов
    resource_num = atoi(temp);

    fgets(temp, 10, in);        // считываем число процессов
    process_num = atoi(temp);

    availResourceVector = (int*) malloc(resource_num * sizeof(int*));
    allocMatrix = (int**) malloc(process_num * sizeof(int*));
    maxMatrix = (int**) malloc(process_num * sizeof(int*));
    needMatrix = (int**) malloc(process_num * sizeof(int*));

    for(int y = 0; y < process_num && !feof(in); y++)
    {
        allocMatrix[y] = (int*) malloc(resource_num * sizeof(int));
        maxMatrix[y] = (int*) malloc(resource_num * sizeof(int));
        needMatrix[y] = (int*) malloc(resource_num * sizeof(int));
    }

    for (int y = 0; y < resource_num; y++) {
        fscanf(in, "%d", &availResourceVector[y]);              // считываем строку ресурсов системы
    }

    for(int y = 0; y < process_num && !feof(in); y++) {
        for(int x = 0; x < resource_num && !feof(in); x++) {
            fscanf(in, "%d", &allocMatrix[y][x]);               // считываем матрицу распределения ресурсов по процессам
        }
    }

    for(int y = 0; y < process_num && !feof(in); y++) {
        for(int x = 0; x < resource_num && !feof(in); x++) {
            fscanf(in, "%d", &maxMatrix[y][x]);                 // считываем матрицу максимального распределения ресурсов по процессам
        }
    }

    fclose(in);
    return 0;
}
