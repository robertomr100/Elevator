//Part 3 - Elevator Scheduling Implementation
//Group 18

#include <linux/printk.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mutex.h>

void Initializer(void);
void PassengersWaiting(int passengertype, int begin, int finish);	
int CheckLoad(void);
int CheckUnload(void);
int Load(int numfloor);
int Unload(void);
int SizeOfWaitingList(int numfloor);
int WeightOfWaitingList(int numfloor);
int ServicedInFloor(int numfloor);
int Moving(int numfloor);
int WeightOfElevator(void);
int SizeOfElevator(void);
int ElevatorThread(void *data);
void ChangeDir(int);

extern int currentFloor;
extern int nextFloor;
extern int stopping;
extern int currentdirection;		//OFFLINE = 0, IDLE = 1, UP = 2, DOWN = 3, LOADING = 4
extern int nextdirection;
extern int attendedpassengers;		

struct list_head currlist;		//list that represents passengers in the elevator

struct Passenger			//represents a passenger waiting in a floor or in the elevator 
{
	struct list_head list;
	int type;
	int weight;			//WEIGHT VALUES ARE DOUBLED IN ORDER TO ACCOUNT FOR .5
	int size;
	int start;
	int finish;
};

struct Floor 				//keeps track of the people waiting in a floor
{
	struct list_head floorQueue;
	int size;
	int weight;
	int serviced;
};

struct Floor AllFloors[10];		//array represents the 10 floors

extern struct task_struct* mainthread;
extern struct mutex passengerLock;//used when accessing the floors data
extern struct mutex elevatorLock;//used when accessing the elevator data

void Initializer(void) 
{
	int i = 0;
	while (i < 10) 
	{
		AllFloors[i].size=0;
		AllFloors[i].weight=0;
		AllFloors[i].serviced=0;
		INIT_LIST_HEAD(&AllFloors[i].floorQueue);//initialize the 10 floors
		i++;
  	}
  	INIT_LIST_HEAD(&currlist);//initialize the elevator
}

int WeightOfWaitingList(int floor) 
{
	int weight = 0;
	
  	mutex_lock_interruptible(&passengerLock);
	weight = AllFloors[floor].weight;//access the weight of the passangers waiting in thr floor
  	mutex_unlock(&passengerLock);

  	return weight;

}

int SizeOfWaitingList(int floor) 
{ 
  	int total = 0;
	

  	mutex_lock_interruptible(&passengerLock);
	
	total = AllFloors[floor].size;//access the size of the people waiting in the floor
  	mutex_unlock(&passengerLock);

  	return total;
}

int ServicedInFloor(int floor)
{
	int total = 0;
	

  	mutex_lock_interruptible(&passengerLock);
	
	total = AllFloors[floor].serviced;//access the people serviced on the floor
  	mutex_unlock(&passengerLock);

  	return total;
}

int WeightOfElevator(void) 
{ 
  	int weight = 0;
	struct Passenger* pass;
  	struct list_head* pos;

  	mutex_lock_interruptible(&elevatorLock);
  	list_for_each(pos, &currlist) //go through the list of the elevator
	{
    		pass = list_entry(pos, struct Passenger, list);
    		weight = weight + pass->weight;//for each passanger in the list add its weight
  	}
  	mutex_unlock(&elevatorLock);

  	return weight;
}

int SizeOfElevator(void) 
{ 
//same as WeightOfElevator but with size
	int total = 0;
	struct Passenger *pass;
	struct list_head *pos;

  	mutex_lock_interruptible(&elevatorLock);
  	list_for_each(pos, &currlist) 
	{
    		pass = list_entry(pos, struct Passenger, list);
    		total = total + pass->size;
  	}
  	mutex_unlock(&elevatorLock);

  	return total;
}



void PassengersWaiting(int type, int start, int end) 
{
	struct Passenger *newPass;
	newPass = kmalloc(sizeof(struct Passenger), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	newPass->type = type; //create a new passenger and add weight and size depending on the type
	if(type==1)
	{
		newPass->weight=2;
		newPass->size=1;
	}

	else if(type==2)
	{
		newPass->weight=1;
		newPass->size=1;
	}

	else if(type==3)
	{
		newPass->weight=4;
		newPass->size=2;
	}
	else if(type==4)
	{
		newPass->weight=8;
		newPass->size=2;
	}
  	
	newPass->start = start;
  	newPass->finish = end;
  	mutex_lock_interruptible(&passengerLock);
	AllFloors[start].weight=AllFloors[start].weight+newPass->weight;
	AllFloors[start].size=AllFloors[start].size+newPass->size;
  	list_add_tail(&newPass->list, &AllFloors[start ].floorQueue);//add the passenger to the list in the respective floor 
  	mutex_unlock(&passengerLock);
  
}

int ElevatorThread(void *data) 
{
while (!kthread_should_stop()) 
{
	if(currentdirection==1)//if idle 
	{
		nextdirection = 2;

              	if(CheckLoad() && stopping==0) //if need to load and not in the process of stopping
          		currentdirection = 4;

		else 
		{
          		currentdirection = 2;
          		nextFloor = currentFloor + 1;
              	}
	}
      	
	else if(currentdirection == 2)//if moving up
	{	
        	Moving(nextFloor);//move to the next floor

		if(stopping == 0)//check if the elevator is stopping
		{
			if(CheckLoad()||CheckUnload())//if need to load or unload
				currentdirection=4;

			else
			{	
				if(currentFloor == 9) 
					ChangeDir(9);//change directions if reached the top floor

				else 
			        	nextFloor = currentFloor + 1;
			}

				

		}

		else
		{
			if(CheckUnload()==1)
				currentdirection=4;

			else
			{
				if(currentFloor == 9) 
					ChangeDir(9);

				else 
			        	nextFloor = currentFloor + 1;

			}
		}
	}
      	
	else if(currentdirection==3)//if moving down, same as up but check for the 0 floor instead
	{
              	Moving(nextFloor);
           
		if(stopping==0)
		{
			if(CheckLoad()==1||CheckUnload()==1)
				currentdirection=4;

			else
			{
				if(currentFloor == 0) 
					ChangeDir(0);
				else if(currentFloor==9)
				{
					if(CheckLoad())
						currentdirection=4;

					else
				        	nextFloor = currentFloor - 1;
								
				}
 
				else 
			        	nextFloor = currentFloor - 1;
			}
					

		}
		
		else
		{
			if(CheckUnload()==1)
				currentdirection=4;
					
			else
			{
				if(currentFloor == 0) 
					ChangeDir(0);

				else 
			        	nextFloor = currentFloor - 1;	
			}

		}
				
	}
        
	else if(currentdirection==4)//if loading
	{
        	ssleep(1);
              	Unload();//unload first
              			
		while (CheckLoad()) //while needing to load
		{
			Load(currentFloor);
		}
              	
		currentdirection = nextdirection;//keep the last direction that the elevator was going
              	
		if (currentdirection == 2) 
		{
			if (currentFloor == 9) 
                  		ChangeDir(9);

			else 
                  		nextFloor = currentFloor + 1;

 		} 
		
		else 
		{
			if (currentFloor == 0) 
				ChangeDir(0);
			else 
				nextFloor = currentFloor - 1;
              	}
	}

}
	return 0;
}



int Moving(int floor) //move to floor
{
	
	if (floor == currentFloor) 
    		return 0;
	
	else 
	{
		ssleep(2);
    		currentFloor = floor;
    		return 1;
  	}
}



int CheckLoad(void) 
{
	struct Passenger *passenger;
	struct list_head *pos;
  
  	if(SizeOfElevator() >= 10)
    		return 0;

  	mutex_lock_interruptible(&passengerLock);
	list_for_each(pos, &AllFloors[currentFloor].floorQueue)//for the first passenger in the list of the current floor 
	{
    		passenger = list_entry(pos, struct Passenger, list);
    		if ((passenger->size + SizeOfElevator() <= 10)&&(passenger->weight + WeightOfElevator() <= 30) 
		&& ((passenger->finish > currentFloor && nextdirection == 2) 
		|| (passenger->finish < currentFloor && nextdirection == 3))) //check if the passenger can fit
		{
      			mutex_unlock(&passengerLock);
      			return 1;//if need to load return 1
    		}
		else
		{
			mutex_unlock(&passengerLock);
      			return 0;
		}
  	}
  	mutex_unlock(&passengerLock);
  	return 0;
}

int CheckUnload(void) 
{
	struct Passenger *x;
  	struct list_head *pos;

  	mutex_lock_interruptible(&elevatorLock);
  	list_for_each(pos, &currlist) 
	{
    		x = list_entry(pos, struct Passenger, list);
    		if (x->finish == currentFloor) 
		{
      			mutex_unlock(&elevatorLock);
      			return 1;//if a single element of the list needs to unload return 1
    		}
  	}
  	mutex_unlock(&elevatorLock);

  	return 0;
}

int Load(int floor) 
{
	int weight = WeightOfElevator();//get the weight, no need to use mutex as the function WeightOFElevator already uses it
				
	struct Passenger *pass;
	struct list_head *pos, *q;
	
	mutex_lock_interruptible(&passengerLock);
	list_for_each_safe(pos, q, &AllFloors[floor].floorQueue) //use the safe in order to delete from the list
	{
		pass = list_entry(pos, struct Passenger, list);

    		if ((pass->start == floor) && ((pass->weight + weight) <= 30)) //check if the passenger wont cause overload 
		{
      			struct Passenger *newPass;//create a new passenger to be added to the elevator list
      			newPass = kmalloc(sizeof(struct Passenger), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
      			newPass->type = pass->type;
      			newPass->weight = pass->weight;
      			newPass->size = pass->size;
      			newPass->start = pass->start;
      			newPass->finish = pass->finish;
			AllFloors[floor].weight=AllFloors[floor].weight-pass->weight;
			AllFloors[floor].size=AllFloors[floor].size-pass->size;//subtract the values from the floor total
      			mutex_lock_interruptible(&elevatorLock);
      			list_add_tail(&newPass->list, &currlist);//add the passenger to the tail
      			list_del(pos);//delete the passanger
      			kfree(pass);
      			mutex_unlock(&elevatorLock);
     			mutex_unlock(&passengerLock);
    		}
  	}
  	mutex_unlock(&passengerLock);

	return 0;
}

int Unload(void) 
{
	struct Passenger *pass;
	struct list_head *pos, *q;

	mutex_lock_interruptible(&elevatorLock);
	list_for_each_safe(pos, q, &currlist) 
	{
		pass = list_entry(pos, struct Passenger, list);
    		if (pass->finish == currentFloor)//unload passengers from elevator list
		{
	    		attendedpassengers++;
			AllFloors[pass->start].serviced++;
      			list_del(pos);//delete from list
      			kfree(pass);
    		}
  	}
  	mutex_unlock(&elevatorLock);

	return 0;
}

void ChangeDir(int floor)//change direction from UP TO DOWN and vice versa
{
	if(CheckLoad() && stopping==0)
		currentdirection=4;
	
	else
	{
		if(floor==0)
		{
			nextdirection = 2;
                	currentdirection = 2;
               		nextFloor = currentFloor + 1;
		}

		if(floor==9)
		{
			nextdirection = 3;
                	currentdirection = 3;
                	nextFloor = currentFloor - 1;
		}

	}
}



