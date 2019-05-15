#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>


//
//////////////////////////////////////////////////
////////////////////////////////////////////////// globals
//////////////////////////////////////////////////

#define JOB_COUNT_MAX 100
#define END_TIME 100

char *jobsFilename = "jobs.dat";

enum JobStatus {
    UNKNOWN,
    RUNNABLE,
    RUNNING,
    DONE
};

typedef struct {
    // job specs
    int id;
    int arrivalTime;
    int duration;
    // scheduler data
    enum JobStatus status;
    int startTime;
    int endTime;

} Job;

//////////////////////////////////////////////////
////////////////////////////////////////////////// functions
//////////////////////////////////////////////////

int readJobs(char *filename, Job *jobs, int maxCount) {
    FILE *f = fopen(filename, "r");
    if(f == NULL) {
        printf(
            "ERROR: cannot open file [%s] for reading: %s\n",
            filename,
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }

    int i;
    for(
        i = 0;
        (
            fscanf(
                f,
                "%d%d%d",
                &jobs[i].id,
                &jobs[i].arrivalTime,
                &jobs[i].duration
            ) != EOF
        ) &&
        (i < maxCount);
        i++
    ) {
        jobs[i].status    = UNKNOWN;
        jobs[i].startTime = -1;
        jobs[i].endTime   = -1;
    }

    return i;
}

int allJobsDone(Job *jobs, int jobCount) {
    for(int i = 0; i < jobCount; i++) {
        if(jobs[i].status != DONE) {
            return 0; // something is not done
        }
    }

    return 1; // all done
}
int printRunLog(Job *jobs, int jobCount, char *schedulerType) {
    printf("Run log for %s:\n", schedulerType);
    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        printf(
            "Job id %02d start/finish %02d - %02d, total %02d, response %02d\n",
            j->id,
            j->startTime,
            j->endTime,
            j->endTime - j->arrivalTime,
            j->startTime - j->arrivalTime
        );
    }

    return 1; // all done
}

int findJobToRunFIFO(Job *jobs, int jobCount) {

    int minArrivalTime = INT_MAX;
    int jobToRunId     = -1;

    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        if((j->status == RUNNABLE) && (j->arrivalTime < minArrivalTime)) {
            minArrivalTime = j->arrivalTime;
            jobToRunId     = i;
        }
    }

    return jobToRunId;
}

void runFIFO(Job *jobs, int jobCount) {
    int runningJobId = -1;
    int runningJobEndTime = -1;

    for(int ticker = 0; ticker < END_TIME; ticker++) {
        printf("runFIFO: ticker [%03d]\n", ticker);
        for(int i = 0; i < jobCount; i++) {
            Job *j = &jobs[i];
            if(
                (j->arrivalTime == ticker) &&
                (j->status != DONE)
            ) {
                printf("runFIFO: \tjob [%d]: setting to RUNNABLE\n", i);
                j->status = RUNNABLE;
            }
        }
        if((runningJobId != -1) && (runningJobEndTime <= ticker)) {
            // end current job
            printf("runFIFO: \tjob [%d]: setting to DONE\n", runningJobId);
            jobs[runningJobId].status = DONE;
            jobs[runningJobId].endTime = ticker;
            runningJobId = -1;
            runningJobEndTime = -1;
        }
        if(runningJobId == -1) {
            // start a new job, if any available
            int jobId = findJobToRunFIFO(jobs, jobCount);
            if(jobId != -1) {
                Job *j            = &jobs[jobId];
                j->status         = RUNNING;
                j->startTime      = ticker;
                runningJobId      = jobId;
                runningJobEndTime = ticker + j->duration;
                printf(
                    "runFIFO: \tjob [%d]: start! end time %d\n",
                    jobId,
                    runningJobEndTime
                );
            } else {
                printf("runFIFO: \tnothing to run\n");
            }
        }
        if(allJobsDone(jobs, jobCount)) {
            printf("runFIFO: ALL JOBS DONE\n");
            break;
        }
    }

    printRunLog(jobs, jobCount, "FIFO");

    return;
}

void runSJF(Job *jobs, int jobCount) {
}

void runBJF(Job *jobs, int jobCount) {
}

void runSTCF(Job *jobs, int jobCount) {
}

void runRR(Job *jobs, int jobCount) {
}

//////////////////////////////////////////////////
////////////////////////////////////////////////// main
//////////////////////////////////////////////////

int main(void) {
    Job jobs[JOB_COUNT_MAX + 1] = {0};
    int jobCount = readJobs(jobsFilename, jobs, JOB_COUNT_MAX);

    runFIFO(jobs, jobCount);
    runSJF(jobs, jobCount);
    runBJF(jobs, jobCount);
    runSTCF(jobs, jobCount);
    runRR(jobs, jobCount);

    return 0;
}

