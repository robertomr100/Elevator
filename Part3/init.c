//Part 3 - Elevator Scheduling Implementation
//Group 18


#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/proc_fs.h> 
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/mutex.h> 
#include "elevator_move.c"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("An elevator simulator utilizing first-in, first-out scheduling.");


#define MODULE_NAME "elevator"
#define PERMISSIONS 0644
#define PARENT NULL

static char * message;
static char * curr;
static char * next;
static char * weight;
static char * Esize;
static char * serviced;
static char * first;
static char * second;
int stopping;
int currentdirection;
int nextdirection;
int currentFloor;
int nextFloor;
int numPassengers;
int numWeight;
int waitPassengers;
int attendedpassengers;


int i;
int read_p;

struct mutex passengerLock;
struct mutex elevatorLock;

struct task_struct* mainthread;

static struct file_operations fileOperations; 

extern int (*STUB_start_elevator)(void);
int start_elevator(void) //start the elevator
{
	if (currentdirection == 0) //if it is offline start it and return 0
	{
		printk("Starting elevator\n");
		currentdirection = 1;

		return 0;
	} 

	else 
	{
		return 1;
	}
}

extern int (*STUB_issue_request)(int,int,int);
int issue_request(int passenger_type, int start_floor, int destination_floor) 
{
	printk("New request: %d, %d => %d\n", passenger_type, start_floor, destination_floor);
	if (start_floor == destination_floor) 
	{
		attendedpassengers++;

	} 
	else 
	{
		if((passenger_type<1||passenger_type>4)||(start_floor<1||start_floor>10)
			||(destination_floor<1||destination_floor>10)||(stopping==1))//check if it is a valid passenger
		{
			return 1;
		}
		PassengersWaiting(passenger_type, start_floor-1, destination_floor-1);//-1 because we are using an array
	} 
	return 0;
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void) 
{
	printk("Stopping elevator\n");
 	if (stopping == 1) //if already stopping return 1
	{
		return 1;
	}
	currentdirection=0;
	stopping = 1;//else start stopping
	return 0;
}


int elevator_open(struct inode *sp_inode, struct file *sp_file) {
 printk(KERN_NOTICE "proc open called\n");
 read_p = 1;
 message = kmalloc(sizeof(char) * 2400, __GFP_RECLAIM |__GFP_WRITE| __GFP_IO | __GFP_FS);
 if(message == NULL) {
 printk("elevator open ERROR");
 return -ENOMEM;
 }

 strcpy(message,".");
 return 0;
}



ssize_t elevator_read(struct file *sp_file, char __user *buff, size_t size, loff_t *offset) {


int n;
numPassengers = SizeOfElevator();
numWeight = WeightOfElevator()/2;
curr= kmalloc(sizeof(char) * 10, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
next= kmalloc(sizeof(char) * 10, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
weight= kmalloc(sizeof(char) * 10, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
Esize= kmalloc(sizeof(char) * 10, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
serviced= kmalloc(sizeof(char) * 10, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
//allocate the memory of strings to be concatenated to message


n = numWeight % 1;
	//used to check for .5


strcat(message, "\nCurrent Elevator Status/Direction: ");
if(currentdirection == 0)
{
	strcat(message, "OFFLINE \n");
}

else if(currentdirection==1)
{
	strcat(message, "IDLE \n");
}

else if(currentdirection==2)
{
	strcat(message, "UP \n");
}

else if(currentdirection==3)
{
	strcat(message, "DOWN \n");
}

else if(currentdirection==4)
{
	strcat(message, "LOADING \n");
}
//add the direction

strcat(message, "\nCurrent floor is: Floor ");
sprintf(curr,"%d \n", currentFloor+1);
strcat(message, curr);

strcat(message, "\nNext floor is: Floor ");
sprintf(next,"%d \n", nextFloor+1);
strcat(message, next);
//add the floors

if (n) 
{

	strcat(message, "Elevator weight: ");
	sprintf(weight,"%d.5 \n", numWeight);
	strcat(message, weight);

} 

else 
{

strcat(message, "Elevator weight: ");
sprintf(weight,"%d \n", numWeight);
strcat(message, weight);

 
}

strcat(message, "Current elevator size: ");
sprintf(Esize,"%d \n", numPassengers);
strcat(message, Esize);

strcat(message, "Passengers attended: ");
sprintf(serviced,"%d \n", attendedpassengers);
strcat(message, serviced);
//add weight, size and serviced
first= kmalloc(sizeof(char) * 2048, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
second= kmalloc(sizeof(char) * 256, __GFP_RECLAIM | __GFP_IO | __GFP_FS);

int SizePrint;
int WeightPrint;
int ServicedPrint;
int i = 0, odd = 0;

strcpy(first,".");

while (i < 10) //loop through all the floors and get the data to print
{
sprintf(second, "\nFloor number %d\n", i+1);
strcat(first, second);
 	//functions to get the weight, size and serviced of each floor
SizePrint = SizeOfWaitingList(i);
WeightPrint = WeightOfWaitingList(i);
ServicedPrint = ServicedInFloor(i);
odd = WeightPrint % 2;
if (odd) {//print the .5 if odd weight
sprintf(second, "Passengers attended: %d\nSize of passengers currently in floor: %d\nWeight of passengers in floor: %d.5\n", ServicedPrint, SizePrint, WeightPrint/2);
} else {
sprintf(second, "Passengers attended: %d\nSize of passengers currently in floor: %d\nWeight of passengers in floor: %d\n", ServicedPrint, SizePrint, WeightPrint/2);
}
strcat(first, second);//concatenate the strings

i++;
}
strcat(first, "\n");

strcat(message, first);//concatenate to message
 



read_p = !read_p;//check if already read
if (read_p) {
return 0;
}

printk(KERN_NOTICE "ReadModule() called.\n");
int len = strlen(message);
copy_to_user(buff, message, len);//output to proc

return len;
}

int elevator_release(struct inode *sp_inode, struct file *sp_file) {
printk(KERN_NOTICE "ReleaseModule() called.\n");

kfree(message);
 kfree(curr);
kfree(next);
kfree(weight);
kfree(Esize);
kfree(serviced);
kfree(first);
kfree(second);
//free memory

return 0;
}


static int InitializeModule(void) {
printk(KERN_NOTICE "Creating /proc/%s.\n", MODULE_NAME);

fileOperations.open = elevator_open;
fileOperations.read = elevator_read;
fileOperations.release = elevator_release;


currentdirection = 0;// initialize variables
nextdirection = 1;
stopping = 0;
currentFloor = 0;
nextFloor = 0;
numPassengers = 0;
numWeight = 0;
waitPassengers = 0;


Initializer();//call to initialize list
STUB_issue_request = &(issue_request);
STUB_start_elevator = &(start_elevator);
STUB_stop_elevator = &(stop_elevator);
mutex_init(&passengerLock);
mutex_init(&elevatorLock);

mainthread = kthread_run(ElevatorThread, NULL, "Elevator Thread");//start the thread
if(IS_ERR(mainthread)) 
{
printk("Error: ElevatorThread\n");
return PTR_ERR(mainthread);
}
if(!proc_create(MODULE_NAME, PERMISSIONS, NULL, &fileOperations))
{
printk("Error: proc_create\n");
remove_proc_entry(MODULE_NAME, NULL);
return -ENOMEM;
}

return 0;
}

module_init(InitializeModule);

static void ExitModule(void) {
int r;
remove_proc_entry(MODULE_NAME, NULL);
STUB_issue_request = NULL;
STUB_start_elevator = NULL;
STUB_stop_elevator = NULL;
mutex_destroy(&passengerLock);
mutex_destroy(&elevatorLock);
printk(KERN_NOTICE "Removing /proc/%s.\n", MODULE_NAME);
r = kthread_stop(mainthread);
if(r != -EINTR) {
printk("Elevator stopped...\n");
}

}


module_exit(ExitModule);
