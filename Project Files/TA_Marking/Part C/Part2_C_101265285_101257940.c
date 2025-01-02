
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>  //Needed for part c when outputing to ta txt files
#include <sys/shm.h>
#include <time.h>

#define totalNumMarks 5           // Number of Teaching Assistants
#define numStudents 20     // Total number of students

// Generate a identifier key from a file and poject ID for the shared memory
#define SHM_KEY 12345

// Array of semaphores to help TAs take turns marking
sem_t taSemaphores[totalNumMarks];

// Function to simulate a TA marking exams
void runMarking(int taID, int *studentList) {

    //Unique seed is performed by reinitialzing from a random number generator
    srand(time(NULL) + taID);

    // Textfile for Ta being created
    char fileName[20];
    sprintf(fileName, "TA%d.txt", taID + 1);
    FILE *taFile = fopen(fileName, "w");
    if (!taFile) {
        perror("Failed to create TA file");
        exit(1);
    }

    int session = 0;
    while (session < 3) { // Each TA performs 3 sessions
        for (int i = 0; i < numStudents; i++) {

            //Get the first semaphore
            sem_wait(&taSemaphores[taID]);

            // Getthe second semaphore
            if (sem_trywait(&taSemaphores[(taID + 1) % totalNumMarks]) != 0) {
                // First semaphore will be used otherwise
                sem_post(&taSemaphores[taID]);
                continue;
            }

            // In this critical section, a random mark is given to the student
            int studentID = studentList[i];
            int markGiven = rand() % 11; // Random mark between 0 and 10

            // Log to the console
            printf("TA %d marked Student %d with %d\n", taID + 1, studentID, markGiven);

            // Write to the TA's file
            fprintf(taFile, "TA %d marked Student %d with %d\n", taID + 1, studentID, markGiven);

            // Perform the delay
            sleep(1 + rand() % 3);

            // Semaphores are released
            sem_post(&taSemaphores[(taID + 1) % totalNumMarks]);
            sem_post(&taSemaphores[taID]);
        }
        session++;
    }

    fclose(taFile); // Close the TA's file
    printf("TA %d finished marking.\n", taID + 1);
}

int main() {
    pid_t taProcesses[totalNumMarks]; // The array to store process IDs due to unique tas
    srand(time(NULL)); // Random number generator is initialized

    // Initializing semaphores
    for (int i = 0; i < totalNumMarks; i++) {
        sem_init(&taSemaphores[i], 1, 1); // Binary semaphore
    }

    // Create shared memory for the list of students
    int shmID = shmget(SHM_KEY, numStudents * sizeof(int), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("Shared memory creation failed");
        exit(1);
    }

    //Createing a shared memory segment with a unique key that has been defined by incorporating IPC_CREAT to ensure
    //the segment is created, and 0666 to set read/write permissions for all users
    int *studentList = (int *)shmat(shmID, NULL, 0);
    if (studentList == (int *)-1) {
        perror("Unable to make shared memory");
        exit(1);
    }

    // Initialize the student list in shared memory
    for (int i = 0; i < numStudents; i++) {
        studentList[i] = 1000 + i; // Student IDs: 1000, 1001, ..., 1019
    }

    // Fork processes for each TA as requested
    for (int i = 0; i < totalNumMarks; i++) {
        taProcesses[i] = fork();
        if (taProcesses[i] < 0) {
            perror("Fork failed");
            exit(1);
        }
        if (taProcesses[i] == 0) { // Check if this is the child process created by fork()
            runMarking(i, studentList);
            exit(0);
        }
    }

    // We must wait for the TA processes to complete
    for (int i = 0; i < totalNumMarks; i++) {
        waitpid(taProcesses[i], NULL, 0);
    }

    // Fully remove from memory
    shmdt(studentList);
    shmctl(shmID, IPC_RMID, NULL);

    // Cleaning up semaphores
    for (int i = 0; i < totalNumMarks; i++) {
        sem_destroy(&taSemaphores[i]);
    }

    printf("Marking is complete.\n");
    return 0;
}