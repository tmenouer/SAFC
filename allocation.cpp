#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <signal.h>
#include <vector>
#include <cassert>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <list>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <climits>


using namespace std;


struct NodeT{ ///Structur to save the Google trace
	int type;
	float priority_cores; //Class SLA quantitative about cores
	float priority_restart; //Class SLA quantitative about the number of restart
	float priority_time; //Class SLA qualitative about satisfaction time
	float priority_security; //Class SLA qualitative about security
	float seq_timeT;
	float start;
	int stop_node;
	float Promethee;
};


typedef struct NodeT NodeT;

struct NodePromethee{
	float priority_cores;
	float priority_time;
};
typedef struct NodePromethee NodePromethee;

struct SPromethee{
	float D[2];
	float P;
};
typedef struct SPromethee SPromethee;


struct Config{
	std::vector<int> nb_nodes;
	int initial; //Initial number of cores
	int id;
	int total; //Total number of cores
	string name;
	string pwd;
	int containers;
	int containers_simulation;
	
	std::vector<int> VUsedCpus;
	std::vector<int> VUsedCpus_Simulation;
	std::vector<int> VNotUsedCpus;
	std::vector<int> type;
	int New_cpus_simulation;
};

//Variables
typedef struct Config Config;

std::vector<Config> VConfig;
std::vector<Config> UsedMachine;
std::vector<Config> PendingMachine;
std::vector<Config> NotUsedMachine;
int PEnergy=0;
std::vector<int> BPacking;
std::vector<int> MinCores_Hard;
std::vector<int> MaxCores_Hard;
std::vector<int> MinCores;
std::vector<int> MaxCores;
std::list<NodeT> ListTaches;
std::vector<int> Simul_cores;
std::vector<int> Simul_iteration;
std::vector<float> Simul_wait;
std::vector<float> Simul_work;
std::vector<float> Simul_work_total;
std::vector<int> Simul_type;
std::vector<int> Simul_cores_type0;
std::vector<int> Simul_cores_type1;
std::vector<int> Simul_cores_type2;
bool stop_work=false;
bool Submition_BD = false;
int Copie_Used_Machine=1;
long int t0;
struct timeval  tv1, tv2, tv3;
std::vector<int> VContainers; //Number of cores allocated to each container

int Submission_id=-1;
int Submission_id_BD=-1;
int Sequential_life_time=0;
int NB_Jobs=0;
int Frequency_number=0;




/// *********** Split functions ************ ///

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
std::stringstream ss(s);
std::string item;
	while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

/// *********** Configuration functions ************ ///

int Config_cores(string name, string pwd)
{

	int nb_cores;
	string commande1,commande2;
	commande1 ="ssh "+ name + " 'cat /proc/cpuinfo | grep processor | wc -l > fichier_info.txt'";
	system(commande1.c_str());
	commande2="scp "+ name + ":fichier_info.txt .";
	system(commande2.c_str());
	ifstream fichier("fichier_info.txt", ios::in);
	if(fichier)  
	{
		string ligne;
		while(getline(fichier, ligne))
		{
			nb_cores=atoi(ligne.c_str());
		}
	}
	
	std::cout<<"Je suis ici fin \n";
	return nb_cores;
}
void Configuration_Simulation()
{
	for(int i=0;i<3;i++)
	{
		Simul_iteration.push_back(0);	
		Simul_cores.push_back(0);
		Simul_wait.push_back(0);
		Simul_work.push_back(0);
		Simul_work_total.push_back(0);
		Simul_type.push_back(0);		
	}
}
void Configuration_Borne()
{
	int small_machine=0;
	int min_best,max_best,min_advanced,max_advanced,min_premium,max_premium;
	  
	/// Computes the number of cores of the small machine 
	small_machine=VConfig[0].total;
	
	for(int i=1;i<VConfig.size();i++)
	{
		if(small_machine>VConfig[i].total)
		{
			small_machine=VConfig[i].total;
		}
	}
	
	///////Factor bound cores ////////////////
	small_machine=small_machine*1;
	///////Factor bound cores ////////////////
	
	
	min_best=1;
	max_best= (int) (small_machine/3);
	
	min_advanced=max_best+1;
	max_advanced=(int) ((small_machine *2)/3);
	
	min_premium=max_advanced+1;
	max_premium=small_machine;
	
	//Oficiel
	//Init Min and Max for each type
	MinCores_Hard.push_back(min_premium);
	MaxCores_Hard.push_back(max_premium);
	
	MinCores_Hard.push_back(min_advanced);
	MaxCores_Hard.push_back(max_advanced);
	
	MinCores_Hard.push_back(min_best);
	MaxCores_Hard.push_back(max_best);
	
	MinCores.push_back(min_premium);
	MaxCores.push_back(max_premium);
	
	MinCores.push_back(min_advanced);
	MaxCores.push_back(max_advanced);
	
	MinCores.push_back(min_best);
	MaxCores.push_back(max_best);
	
	
	for(int i=0;i<3;i++)
	{
		if(MinCores_Hard[i]<1)
		{
			MinCores_Hard[i]=1;
		}
		if(MaxCores_Hard[i]<1)
		{
			MaxCores_Hard[i]=1;
		}
		
		if(MinCores[i]<1)
		{
			MinCores[i]=1;
		}
		if(MaxCores[i]<1)
		{
			MaxCores[i]=1;
		}
		std::cout<<" Type "<<i<<" Min "<<MinCores_Hard[i]<<" max "<<MaxCores_Hard[i]<<"\n";
	}
}

void Configuration_Start_Machines()
{
	Config C_big_machine;
	
	C_big_machine.total=VConfig[0].total;
	C_big_machine.id=0;
	C_big_machine.initial=VConfig[0].initial;
	C_big_machine.pwd=VConfig[0].pwd;
	C_big_machine.name=VConfig[0].name;
	C_big_machine.type=VConfig[0].type;
	C_big_machine.containers=VConfig[0].containers;
	C_big_machine.containers_simulation=VConfig[0].containers_simulation;
	
	for(int i=1;i<VConfig.size();i++)
	{
		if(C_big_machine.total<VConfig[i].total)
		{
			C_big_machine.total=VConfig[i].total;
			C_big_machine.initial=VConfig[i].initial;
			C_big_machine.id=i;
			C_big_machine.pwd=VConfig[i].pwd;
			C_big_machine.name=VConfig[i].name;
			C_big_machine.type=VConfig[i].type;
			C_big_machine.containers=VConfig[i].containers;
			C_big_machine.containers_simulation=VConfig[i].containers_simulation;
		}
	}
	
	///Inserer la grand machine dasn UseMachine
	UsedMachine.push_back(C_big_machine);
	PEnergy +=5;
	
	///Inserer toutes les autres machines dans NotUsedMachine ou UsedMachines=
	for(int i=0;i<VConfig.size();i++)
	{
		if(VConfig[i].id != C_big_machine.id)
		{
			Config CC;
			CC.total=VConfig[i].total;
			CC.initial=VConfig[i].initial;
			CC.id=i;
			CC.pwd=VConfig[i].pwd;
			CC.name=VConfig[i].name;
			CC.type=VConfig[i].type;
			CC.containers=VConfig[i].containers;
			CC.containers_simulation=VConfig[i].containers_simulation;
			//~ NotUsedMachine.push_back(CC);
			UsedMachine.push_back(CC);
		}
	}
	
	//Affichage des machines
	for(int i=0;i<UsedMachine.size();i++)
	{
		std::cout<<"Used avec Id "<<UsedMachine[i].id<<" total "<< UsedMachine[i].total<<" initial "<<UsedMachine[i].initial<<" name "<<UsedMachine[i].name<<"\n";
	}
	
	for(int i=0;i<NotUsedMachine.size();i++)
	{
		std::cout<<"NOT Used avec Id "<<NotUsedMachine[i].id<<" total "<< NotUsedMachine[i].total<<" initial "<<NotUsedMachine[i].initial<<"name "<<NotUsedMachine[i].name<<"\n";
	}
}

void Configuration()
{

  int cmpt=0;
  std::vector<string> s;
  char buffer_rm[1024];
  
  snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info.txt fichier_info2.txt");		
  system(buffer_rm);

  ifstream fichier("ConfigG5000.txt", ios::in);  // ConfigG5000 is file which contains all the infrastructure machines with name and pwd
  if(fichier)  
  {
	string ligne;
	while(getline(fichier, ligne))
	{
		Config C;
		s= split(ligne, ' ');
		C.name=s[0];
		C.pwd=s[1];
		C.total=Config_cores(C.name,C.pwd);
		C.initial=C.total;
		C.id=cmpt;
		
		C.type.push_back(0);
		C.type.push_back(0);
		C.type.push_back(0);
		C.containers=0;
		C.containers_simulation=0;
		cmpt++;
		VConfig.push_back(C);	
	}	      
	fichier.close();
  }
  else  // sinon
	cerr << "Impossible to open the file ConfigG5000 !" << endl;

   Configuration_Simulation();
   Configuration_Borne();
   Configuration_Start_Machines();
	t0=clock();
	gettimeofday(&tv1, NULL);

}

/// ******************* Submission functions *********************
void convert_Prezi()
{
	std::vector<string> s;
	std::vector<string> s_h;	
	ifstream fichier("./JobsBD/Jobs.txt", ios::in);  // on ouvre le fichier en lecture
	ofstream fichier_E("./JobsBD/Jobs_New.txt", ios::out | ios::trunc);  //ouverture en écriture
	int debut,type;
	float time;
	int cmpt=0;
	int copie_time=0;
	int time_0,time_1,time_2;
	time_0=0;
	time_1=0;
	time_2=0;
	
	if(fichier) 
	{
		string ligne;
		while(getline(fichier, ligne))
		{
			s= split(ligne, ' ');
			
			//S[1]=heure
			s_h= split(s[1], ':');
			
			debut=atoi(s_h[2].c_str())+atoi(s_h[1].c_str())*60+atoi(s_h[0].c_str())*3600;
			
			if(s.size() !=0)
			{
				if(s[3]=="general")
				{
					type=0;
				}
				if(s[3]=="export")
				{
					type=1;
				}
				
				if(s[3]=="url")
				{
					type=2;
				}
				
				
				time=atof(s[4].c_str());
				
				if(type==0)
				{
					time_0 += time;
				}else
				{
					if(type==1)
					{
						time_1 += time;
					}else
					{
						time_2 += time;
					}
				}
				
				fichier_E<<debut<<" " <<type<<" "<<time<<"\n";
				cmpt++;
			}
		}
	
	
	
		
		NB_Jobs=cmpt;
		std::cout<<"TIME 0 "<<time_0<<" TIME 1 "<<time_1<<" TIME 2 "<<time_2<<"\n";
		
		fichier.close();
		fichier_E.close();
	}else
	{
		printf("Erreur avec l'ouverture du fichier \n");
	}

}
bool my_compare_start (NodeT a, NodeT b)
{
    return a.start < b.start;
}

void convert_Google()
{
	std::vector<string> s;
	std::list<NodeT> ListGoogle;	
	ifstream fichier("./JobsBD/Jobs_Google.txt", ios::in);  // on ouvre le fichier en lecture
	ofstream fichier_E("./JobsBD/Jobs_New.txt", ios::out | ios::trunc);  //ouverture en écriture
	float time;
	int cmpt=0;
	double x,y;
	int z;
	int type;

	
	if(fichier)  // si l'ouverture a fonctionné
	{
		string ligne;
		while(getline(fichier, ligne))
		{
			s= split(ligne, ',');
			if(cmpt !=0)
			{
				
				if(s.size() !=0 && s.size()<=5)
				{
					type=atoi(s[4].c_str());
					if( type >=0 && type <=2)
					{
						NodeT N;
						if(type == 0)
						{
							N.type=0;
						}else
						{
							if(type == 1)
							{
								N.type=2;
							}else
							{
								N.type=1;
							}
						}
					
						x=atof(s[3].c_str())/100000000;
					
						N.seq_timeT= (int) x;
						
						y=atof(s[1].c_str())/100000000;
						N.start= (int) y;
						//~ fichier_E<<"start "<<N.start<<" et "<< s[1] <<" et atoi "<< atof(s[1].c_str())<<"\n";
						ListGoogle.push_back(N);
						cmpt++;
					}
				}
			}
			if(cmpt==0)
			{
				cmpt++;
			}
			
		}
		
		
		NB_Jobs=cmpt-1;
		//Trie la liste suivant le time start
		ListGoogle.sort(my_compare_start);
		
		std::list<NodeT>::iterator it;
		it= ListGoogle.begin();
		int copie_time=1;

		while (it != ListGoogle.end()) {
			fichier_E<<it->start<<" " <<it->type<<" "<<it->seq_timeT<<"\n";
			copie_time=it->start;
			it++;
		}
	
		fichier.close();
		fichier_E.close();
	}else
	{
		printf("Erreur avec l'ouverture du fichier \n");
	}

}
///Increase la priorité de toutes ls requetes avant la soumisssion d'une nouvelle requete

void Increase_priority()
{
	std::list<NodeT>::iterator it;
	it= ListTaches.begin();

	while (it != ListTaches.end()) {
		switch(it->type)
		{
			case 0: it->priority_time=it->priority_time*1.3;break;
			case 1: it->priority_time=it->priority_time*1.2;break;
			case 2: it->priority_time=it->priority_time*1.1;break;
		}
		it++;
	}
}
///La fonction qui permet de soumetre des requestes Prezi
///La fonction qui permet de soumetre des requestes Prezi
void submitions_BD(int id){
	srand(time(NULL));
	
	std::vector<string> s;
	int type_time;
	int cmpt=0;
	if(id==0)
	{
		std::cout<<"Je sui Prezi ********* \n";
	 convert_Prezi();
	}else
	{	
		std::cout<<"Je sui Google ********* \n";
	 convert_Google();
	}
	 
	for(int i=0;i<NB_Jobs;i++) //Init all containers to 0 core
	{
		VContainers.push_back(0);
	}
	
	std::cout<<"COUCOU JE SUIS LA \n";
	// start time - priorité - sequantial time
	ifstream fichier("./JobsBD/Jobs_New.txt", ios::in);  // on ouvre le fichier en lecture
	if(fichier)  // si l'ouverture a fonctionné
	{
		string ligne;
		while(getline(fichier, ligne))
		{
			s= split(ligne, ' ');
	
			if(s.size() !=0)
			{
				NodeT N;
				N.stop_node=0;
				if(cmpt==atoi(s[0].c_str()))
				{				
					N.type=atoi(s[1].c_str());
					
					switch(N.type)
					{
						case 0: N.priority_cores=3; break;
						case 1: N.priority_cores=2; break;
						case 2: N.priority_cores=1; break;
					}
					
					//~ type_time=rand()%3;
					type_time =N.type;
					switch(type_time)
					{
						case 0: N.priority_time=3; break;
						case 1: N.priority_time=2; break;
						case 2: N.priority_time=1; break;
					}
					
					
					//~ N.seq_timeT=atof(s[2].c_str());
					N.seq_timeT=Sequential_life_time;
					
					gettimeofday(&tv3, NULL);
					//~ N.start= float (clock() - t0) / CLOCKS_PER_SEC;
					N.start = (double) (tv3.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv3.tv_sec - tv1.tv_sec);
					///Incrementer la priority de toutes les requetes
					Increase_priority();
					//Insererer le noeud dans la file
					std::cout<<"Sumission time "<<cmpt<<" Type"<< N.type<<" priority_time "<<N.priority_time<<" priority_cores "<<N.priority_cores <<" Time "<<N.seq_timeT<<"\n";
					
					ListTaches.push_back(N);
				}else
				{
					int t_wait= atoi(s[0].c_str())-cmpt;
					sleep(t_wait);
					cmpt=atoi(s[0].c_str());
										
					N.type=atoi(s[1].c_str());
					
					switch(N.type)
					{
						case 0: N.priority_cores=3; break;
						case 1: N.priority_cores=2; break;
						case 2: N.priority_cores=1; break;
					}
					//~ type_time=rand()%3;
					type_time=N.type;
					switch(type_time)
					{
						case 0: N.priority_time=3; break;
						case 1: N.priority_time=2; break;
						case 2: N.priority_time=1; break;
					}
					
					
					//~ N.seq_timeT=atof(s[2].c_str());
					N.seq_timeT=Sequential_life_time;
					
					
					gettimeofday(&tv3, NULL);
					//~ N.start= float (clock() - t0) / CLOCKS_PER_SEC;
					N.start = (double) (tv3.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv3.tv_sec - tv1.tv_sec);				///Incrementer la priority de toutes les requetes
					Increase_priority();
					//Insererer le noeud dans la file
					std::cout<<" Sumission time "<<cmpt<<" Type"<< N.type<<" priority_time "<<N.priority_time<<" priority_cores "<<N.priority_cores <<" Time "<<N.seq_timeT<<"\n";
					
					ListTaches.push_back(N);
				}
			}
		}
		fichier.close();
	}else
	{
		printf("Erreur avec l'ouverture du fichier \n");
	}
	
	std::cout<<"J'ai fini la fonction \n";
}


///La fonction qui permet de soumetre des taches aleatoire
void submissions_same(){
	
	srand(time(NULL));
	int nb=0;
	for(int i=0;i<NB_Jobs;i++) //Init all containers to 0 core
	{
		VContainers.push_back(0);
	}
	int pas= NB_Jobs/3;
	
	for(int i=0;i<pas;i++)
	{
		NodeT N;
		N.stop_node=0;
		N.type=0;
		N.priority_cores=3;
		N.priority_time=3;
		N.priority_restart=1;
		N.priority_security=3;
		N.seq_timeT=Sequential_life_time;
		N.start= 0;
		///Incrementer la priority de toutes les requetes
		//~ Increase_priority();
		ListTaches.push_back(N);
		nb++;
	}
	
	for(int i=0;i<pas;i++)
	{
		NodeT N;
		N.stop_node=0;
		N.type=1;
		N.priority_cores=2;
		N.priority_time=2;
		N.priority_restart=1;
		N.priority_security=2;
		N.seq_timeT=Sequential_life_time;
		N.start=0;
		///Incrementer la priority de toutes les requetes
		//~ Increase_priority();
		//Insererer le noeud dans la file
		ListTaches.push_back(N);
		nb++;
	}
	
	for(int i=2*pas;i<NB_Jobs;i++)
	{
		NodeT N;
		N.stop_node=0;
		N.type=2;
		N.priority_cores=1;
		N.priority_time=1;
		N.priority_restart=1;
		N.priority_security=1;
		N.seq_timeT=Sequential_life_time;
		N.start=0;
		///Incrementer la priority de toutes les requetes
		//~ Increase_priority();
		//Insererer le noeud dans la file
		ListTaches.push_back(N);
		nb++;
	}
	
	std::cout<<"FIN de SOUMISSION de "<<nb<<" et taille de liste "<< ListTaches.size()<<"\n";
}
///La fonction qui permet de soumetre des taches aleatoire
void submitions_pas(){
	
	srand(time(NULL));
	int nb=0;
	float time_s;
	int type_time;
	int cmpt=0;
	
	for(int i=0;i<NB_Jobs;i++) //Init all containers to 0 core
	{
		VContainers.push_back(0);
	}

	
	while(nb < NB_Jobs)
	{
		time_s=float (clock() - t0) / CLOCKS_PER_SEC;
		for(int i=0;i<3;i++)
		{
			NodeT N;
			N.stop_node=0;
			N.type=i;
			switch(N.type)
			{
				case 0: N.priority_cores=3; break;
				case 1: N.priority_cores=2; break;
				case 2: N.priority_cores=1; break;
			}
			//~ type_time=rand()%3;
			switch(N.type)
			{
				case 0: N.priority_time=3; break;
				case 1: N.priority_time=2; break;
				case 2: N.priority_time=1; break;
			}
			
			switch(N.type)
			{
				case 0: N.priority_restart=1; break;
				case 1: N.priority_restart=1; break;
				case 2: N.priority_restart=1; break;
			}
					
			switch(N.type)
			{
				case 0: N.priority_security=3; break;
				case 1: N.priority_security=2; break;
				case 2: N.priority_security=1; break;
			}
			
			//~ N.seq_timeT=rand()%20+10;
			N.seq_timeT=Sequential_life_time;
			gettimeofday(&tv3, NULL);
			//~ N.start= float (clock() - t0) / CLOCKS_PER_SEC;
			N.start = (double) (tv3.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv3.tv_sec - tv1.tv_sec);
			///Incrementer la priority de toutes les requetes
			Increase_priority();		
			//Insererer le noeud dans la file
			ListTaches.push_back(N);
		}

		cmpt++;
		sleep(Frequency_number);
		//~ usleep(500);
		nb +=3;
	}
	std::cout<<"FIN de SOUMISSION de "<<nb<<"\n";
}
void submitions(){
	
	srand(time(NULL));
	int nb=0;
	int type_time;
	
	while(nb < NB_Jobs)
	{
		NodeT N;
		N.stop_node=0;
		N.type=nb%3;
		switch(N.type)
		{
			case 0: N.priority_cores=3; break;
			case 1: N.priority_cores=2; break;
			case 2: N.priority_cores=1; break;
		}
		
		type_time=rand()%3;
		switch(type_time)
		{
			case 0: N.priority_time=3; break;
			case 1: N.priority_time=2; break;
			case 2: N.priority_time=1; break;
			
		}
		
		switch(rand()%3)
		{
			case 0: N.priority_restart=1; break;
			case 1: N.priority_restart=1; break;
			case 2: N.priority_restart=1; break;
		}
					
		switch(rand()%3)
		{
			case 0: N.priority_security=3; break;
			case 1: N.priority_security=2; break;
			case 2: N.priority_security=1; break;
		}
		//~ N.seq_timeT=rand()%20+10;
		N.seq_timeT=30;
		gettimeofday(&tv3, NULL);
		//~ N.start= float (clock() - t0) / CLOCKS_PER_SEC;
		N.start = (double) (tv3.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv3.tv_sec - tv1.tv_sec);
		///Incrementer la priority de toutes les requetes
		Increase_priority();
		
		std::cout<<"Job "<<nb<<" type "<< N.type <<"\n";
		//Insererer le noeud dans la file
		ListTaches.push_back(N);
		//sleep(1);
		//~ usleep(500);	
		nb++;
	}
	std::cout<<"FIN de SOUMISSION de "<<nb<<"\n";
}

/// ************* Simulation functions ****************


bool check_cpus_simulation(int id_machine, int id_cpu)
{
	bool result = false;
	
	for(int i=0;i <UsedMachine[id_machine].VUsedCpus.size();i++)
	{
		if(UsedMachine[id_machine].VUsedCpus[i] == id_cpu)
		{
			result = true;
		}
	}
	
	return result;
}

void CPUs_Simulation()
{
	
	string commande1,commande2;
	char buffer1[1024];
	char buffer2[1024];
	char buffer_rm[1024];

	int cmpt=0;
	std::vector<string> s1;
	std::vector<string> s2;
	int old=0;
	
	
	for(int i=0;i<UsedMachine.size();i++)
	{
	
		UsedMachine[i].VUsedCpus_Simulation.clear();
		
		snprintf(buffer_rm, sizeof(buffer_rm), "ssh %s 'rm fichier_info_simulation.txt' ", UsedMachine[i].name.c_str());
		system(buffer_rm);
		
		snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info_simulation.txt");
		system(buffer_rm);
		
		snprintf(buffer1, sizeof(buffer1), "ssh %s 'lxc-ls --running > fichier_info_simulation.txt' ", UsedMachine[i].name.c_str());
		system(buffer1);
		
		snprintf(buffer2, sizeof(buffer2), "scp %s:fichier_info_simulation.txt . ", UsedMachine[i].name.c_str());
		system(buffer2);
		
		snprintf(buffer_rm, sizeof(buffer_rm), "ssh %s 'rm fichier_info2_simulation.txt' ", UsedMachine[i].name.c_str());
		system(buffer_rm);

			
		ifstream fichier("fichier_info_simulation.txt", ios::in);
		cmpt=0;
		if(fichier)  
		{
			string ligne;
			UsedMachine[i].New_cpus_simulation=0;
			while(getline(fichier, ligne))
			{
				UsedMachine[i].New_cpus_simulation += VContainers[atoi(ligne.c_str())];
				if(cmpt==0)
				{					
					snprintf(buffer1, sizeof(buffer1), "ssh %s 'lxc-cgroup -n %s cpuset.cpus > fichier_info2_simulation.txt' ", UsedMachine[i].name.c_str(),ligne.c_str());
					
				}else
				{
					snprintf(buffer1, sizeof(buffer1), "ssh %s 'lxc-cgroup -n %s cpuset.cpus >> fichier_info2_simulation.txt' ", UsedMachine[i].name.c_str(),ligne.c_str());
				}
				cmpt++;
				
				system(buffer1);
		
			}
		}
		
		UsedMachine[i].containers_simulation=cmpt;
		
		snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info_simulation.txt");		
		system(buffer_rm);
		
		
		snprintf(buffer2, sizeof(buffer2), "scp %s:fichier_info2_simulation.txt . ", UsedMachine[i].name.c_str());
		system(buffer2);
		
		old=0;
		ifstream fichier2("fichier_info2_simulation.txt", ios::in);
		if(fichier2)  
		{
			string ligne;
			while(getline(fichier2, ligne))
			{
				s1= split(ligne,',');
				if(s1.size()==1)
				{
					s2= split(s1[0],'-');
					if(s2.size()==1)
					{
						if(check_cpus_simulation(i,atoi(s2[0].c_str()))==false)
						{
							UsedMachine[i].VUsedCpus_Simulation.push_back(atoi(s2[0].c_str()));
						}
					}else
					{
					
						for(int j=atoi(s2[0].c_str());j<=atoi(s2[1].c_str());j++)
						{
							if(check_cpus_simulation(i,j)==false)
							{
								UsedMachine[i].VUsedCpus_Simulation.push_back(j);
							}
						}
					}
				}else
				{
					for(int k=0;k<s1.size();k++)
					{
						s2= split(s1[k],'-');
						if(s2.size()==1)
						{
							if(check_cpus_simulation(i,atoi(s2[0].c_str()))==false)
							{
								UsedMachine[i].VUsedCpus_Simulation.push_back(atoi(s2[0].c_str()));
							}
						}else
						{
							for(int j=atoi(s2[0].c_str());j<=atoi(s2[1].c_str());j++)
							{
								if(check_cpus_simulation(i,j)==false)
								{
									UsedMachine[i].VUsedCpus_Simulation.push_back(j);
								}
							}	
						}
					}
				}
			}
			
			if(UsedMachine[i].VUsedCpus_Simulation.size() > UsedMachine[i].initial)
			{
				int dif=UsedMachine[i].VUsedCpus_Simulation.size()-UsedMachine[i].initial;
				
				for(int k=0;k <dif;k++)
				{
					UsedMachine[i].VUsedCpus_Simulation.pop_back();
				}
			}
		}
		//Effacer fichier 2
		snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info2_simulation.txt");		
		system(buffer_rm);
		
		snprintf(buffer_rm, sizeof(buffer_rm), "ssh %s 'rm fichier_info2_simulation.txt' ", UsedMachine[i].name.c_str());
		system(buffer_rm);
	}
}
////Simulation
void simulation()
{
	stop_work=false;
	int cmpt=0;
	ofstream fichier_S("./JobsBD/Simulations.txt", ios::out);  //ouverture en écriture
	if(fichier_S)  // si l'ouverture a fonctionné
	{
		
		while(stop_work == false)
		{
			sleep(5);
			
			//~ CPUs_Simulation();
			//~ for(int i=0;i<UsedMachine.size();i++)
			//~ {
				//~ fichier_S<<UsedMachine[i].VUsedCpus_Simulation.size()<<" ";
				
			//~ }
			for(int i=0;i<UsedMachine.size();i++)
			{
				fichier_S<<UsedMachine[i].VUsedCpus.size()<<" ";
			}
			
			//~ for(int i=0;i<UsedMachine.size();i++)
			//~ {
				//~ fichier_S<<UsedMachine[i].New_cpus_simulation<<" ";
			//~ }
			
			for(int i=0;i<UsedMachine.size();i++)
			{
				fichier_S<<UsedMachine[i].containers_simulation <<" ";
			}
			
			for(int i=0;i<UsedMachine.size();i++)
			{
				fichier_S<<UsedMachine[i].containers<<" ";
			}

			fichier_S<<"\n";
			cmpt++;
		}

		std::cout<<"\n ";
		 for(int k=0;k<VConfig.size();k++)
		{
			fichier_S<<"** Je suis cluster "<<k<<" avec "<<VConfig[k].total<<"\n";
		}
		for(int i=0;i<3;i++)
		{
			if(Simul_iteration[i] !=0)
			{
				fichier_S<<" size "<< Simul_iteration[i] <<" cores "<<i<<" = "<<  Simul_cores[i]<<" wait "<< Simul_wait[i]<<" work "<< Simul_work[i]/1000000<<" work T "<< Simul_work_total[i]<<"\n";
			}
		}
		fichier_S.close();
	}else
	{
		std::cout<<"Probleme avec le fichier simulation \n";
	}
		
	
}


/// ******************* Allocation functions ********************


void add_machine()
{
	int remove;
	
	if(PendingMachine.size() >0)
	{
		Config MaxP;
		MaxP.initial=PendingMachine[0].initial;
		MaxP.total=PendingMachine[0].total;
		MaxP.id=PendingMachine[0].id;
		MaxP.pwd=PendingMachine[0].pwd;
		MaxP.name=PendingMachine[0].name;
		remove=0;
		
		for(int i=1;i<PendingMachine.size();i++)
		{
			if(MaxP.initial < PendingMachine[i].initial)
			{
				MaxP.initial=PendingMachine[i].initial;
				MaxP.total=PendingMachine[i].total;
				MaxP.id=PendingMachine[i].id;
				MaxP.pwd=PendingMachine[i].pwd;
				MaxP.name=PendingMachine[i].name;
				remove=i;
			}
		}
		UsedMachine.push_back(MaxP);
		PendingMachine.erase (PendingMachine.begin()+remove);
		
	}else
	{
		if(NotUsedMachine.size() > 0)
		{
			Config Max;
			Max.initial=NotUsedMachine[0].initial;
			Max.total=NotUsedMachine[0].total;
			Max.id=NotUsedMachine[0].id;
			Max.pwd=NotUsedMachine[0].pwd;
			Max.name=NotUsedMachine[0].name;
			remove=0;
		
			for(int i=1;i<NotUsedMachine.size();i++)
			{
				if(Max.initial < NotUsedMachine[i].initial)
				{
					Max.initial=NotUsedMachine[i].initial;
					Max.total=NotUsedMachine[i].total;
					Max.id=NotUsedMachine[i].id;
					Max.pwd=NotUsedMachine[i].pwd;
					Max.name=NotUsedMachine[i].name;
					remove=i;
				}
			}
			UsedMachine.push_back(Max);
			PEnergy +=5;
			NotUsedMachine.erase (NotUsedMachine.begin()+remove);
		}
	}
}

void remove_machine()
{
	int remove;
	if(UsedMachine.size()>1 )
	{
		Config Min;
		Min.initial=UsedMachine[0].initial;
		Min.total=UsedMachine[0].total;
		Min.id=UsedMachine[0].id;
		Min.pwd=UsedMachine[0].pwd;
		Min.name=UsedMachine[0].name;
		
		remove=0;
		
		for(int i=1;i<UsedMachine.size();i++)
		{
			if(Min.total < UsedMachine[i].total)
			{
				Min.initial=UsedMachine[i].initial;
				Min.total=UsedMachine[i].total;
				Min.id=UsedMachine[i].id;
				Min.pwd=UsedMachine[i].pwd;
				Min.name=UsedMachine[i].name;
				
				remove=i;
			}
		}
		
		if(Min.total == Min.initial)
		{
			NotUsedMachine.push_back(Min);
			//~ PEnergy +=5;
		}else
		{
			PendingMachine.push_back(Min);
		}
		
		UsedMachine.erase (UsedMachine.begin()+remove);
	}
}
bool check_cpus(int id_machine, int id_cpu)
{
	bool result = false;
	
	for(int i=0;i <UsedMachine[id_machine].VUsedCpus.size();i++)
	{
		if(UsedMachine[id_machine].VUsedCpus[i] == id_cpu)
		{
			result = true;
		}
	}
	
	return result;
}

void Adapt_borns(int taux)
{
	if(taux >=0 && taux <=33)
	{
		for(int i=0;i<3;i++)
		{
			MinCores[i]= 2*((MaxCores_Hard[i]-MinCores_Hard[i])/3)+MinCores_Hard[i];
			MaxCores[i]=MaxCores_Hard[i];
		}
	}else
	{
		if(taux>33 && taux <=66)
		{
			for(int i=0;i<3;i++)
			{
				MinCores[i]= 1*((MaxCores_Hard[i]-MinCores_Hard[i])/3)+MinCores_Hard[i];
				MaxCores[i]=2*((MaxCores_Hard[i]-MinCores_Hard[i])/3)+MinCores_Hard[i];
			}
			
		}else
		{
			for(int i=0;i<3;i++)
			{
				MinCores[i]= MinCores_Hard[i];
				MaxCores[i]=1*((MaxCores_Hard[i]-MinCores_Hard[i])/3)+MinCores_Hard[i];
			}
		}
	}
	
	//~ if(taux >=50)
	//~ {
		//~ add_machine();
		//~ std::cout<<"%%%%%%%%%%%%%%%%%%% JE SUIS ADD MACHINE"<<UsedMachine.size()<<"\n";
	//~ }else
	//~ {
		//~ if(taux <=15)
		//~ {
			//~ remove_machine();
		//~ }
	//~ }
}

int CPUs_2()
{
	int taux;
	int initial_global=0;
	int total_global=0;
	string commande1,commande2;
	
	
	char buffer1[1024];
	char buffer2[1024];
	char buffer_rm[1024];
	
	int nb_free_cores;
	int cmpt=0;
	std::vector<string> s1;
	std::vector<string> s2;
	int Nombre_cpus=0;
	
	
	for(int i=0;i<UsedMachine.size();i++)
	{
		initial_global += UsedMachine[i].initial;
		UsedMachine[i].VUsedCpus.clear();
		UsedMachine[i].VNotUsedCpus.clear();
		UsedMachine[i].type[0]=0;
		UsedMachine[i].type[1]=0;
		UsedMachine[i].type[2]=0;
		
		snprintf(buffer_rm, sizeof(buffer_rm), "ssh %s 'rm fichier_info.txt' ", UsedMachine[i].name.c_str());
		system(buffer_rm);
		
		snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info.txt");
		system(buffer_rm);
		
		snprintf(buffer1, sizeof(buffer1), "ssh %s 'lxc-ls --running > fichier_info.txt' ", UsedMachine[i].name.c_str());
		system(buffer1);
		
		snprintf(buffer2, sizeof(buffer2), "scp %s:fichier_info.txt . ", UsedMachine[i].name.c_str());
		system(buffer2);
		
		snprintf(buffer_rm, sizeof(buffer_rm), "ssh %s 'rm fichier_info2.txt' ", UsedMachine[i].name.c_str());
		system(buffer_rm);

			
		ifstream fichier("fichier_info.txt", ios::in);
		cmpt=0;
		if(fichier)  
		{
			string ligne;
			while(getline(fichier, ligne))
			{
				Nombre_cpus += VContainers[atoi(ligne.c_str())];
				
				if(cmpt==0)
				{					
					snprintf(buffer1, sizeof(buffer1), "ssh %s 'lxc-cgroup -n %s cpuset.cpus > fichier_info2.txt' ", UsedMachine[i].name.c_str(),ligne.c_str());
					
				}else
				{
					snprintf(buffer1, sizeof(buffer1), "ssh %s 'lxc-cgroup -n %s cpuset.cpus >> fichier_info2.txt' ", UsedMachine[i].name.c_str(),ligne.c_str());
				}
				cmpt++;
				system(buffer1);
		
			}
		}
		///Fixer le nombre de container 
		UsedMachine[i].containers=cmpt;
		
		snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info.txt");		
		system(buffer_rm);
		
		
		snprintf(buffer2, sizeof(buffer2), "scp %s:fichier_info2.txt . ", UsedMachine[i].name.c_str());
		system(buffer2);
		
		ifstream fichier2("fichier_info2.txt", ios::in);
		if(fichier2)  
		{
			string ligne;
			while(getline(fichier2, ligne))
			{
				s1= split(ligne,',');
				if(s1.size()==1)
				{
					s2= split(s1[0],'-');
					if(s2.size()==1)
					{
						
						if(check_cpus(i,atoi(s2[0].c_str()))==false)
						{
							UsedMachine[i].VUsedCpus.push_back(atoi(s2[0].c_str()));
						}
					}else
					{
					
						for(int j=atoi(s2[0].c_str());j<=atoi(s2[1].c_str());j++)
						{
							if(check_cpus(i,j)==false)
							{
								UsedMachine[i].VUsedCpus.push_back(j);
							}
						}
					}
				}else
				{
					for(int k=0;k<s1.size();k++)
					{
						s2= split(s1[k],'-');
						if(s2.size()==1)
						{
							if(check_cpus(i,atoi(s2[0].c_str()))==false)
							{
								UsedMachine[i].VUsedCpus.push_back(atoi(s2[0].c_str()));
							}
						}else
						{
							for(int j=atoi(s2[0].c_str());j<=atoi(s2[1].c_str());j++)
							{	
								if(check_cpus(i,j)==false)
								{
									UsedMachine[i].VUsedCpus.push_back(j);
								}
							}	
						}
					}
				}
			}
		}
		//Effacer fichier 2
		snprintf(buffer_rm, sizeof(buffer_rm), "rm fichier_info2.txt");		
		system(buffer_rm);
		
		snprintf(buffer_rm, sizeof(buffer_rm), "ssh %s 'rm fichier_info2.txt' ", UsedMachine[i].name.c_str());
		system(buffer_rm);

		
		///Create the NotUsedVector
		bool trouver=false;
		for(int j=0;j<UsedMachine[i].initial;j++)
		{
			trouver=false;
			for(int k=0;k<UsedMachine[i].VUsedCpus.size();k++)
			{
				if(UsedMachine[i].VUsedCpus[k]==j)
				{
					trouver=true;
				}
			}
			if(trouver==false)
			{
				UsedMachine[i].VNotUsedCpus.push_back(j);
			}
		}
		
		
		if(UsedMachine[i].VUsedCpus.size() > UsedMachine[i].initial)
		{
			UsedMachine[i].VNotUsedCpus.clear();
			total_global += UsedMachine[i].initial;
			UsedMachine[i].total = 0;
			int diff=UsedMachine[i].VUsedCpus.size()-UsedMachine[i].initial;
			for(int k=0;k<diff;k++)
			{
				UsedMachine[i].VUsedCpus.pop_back();
			}
			
		}else
		{
			total_global += UsedMachine[i].VUsedCpus.size();
			UsedMachine[i].total = UsedMachine[i].VNotUsedCpus.size();
		}
	}
	
	if(Nombre_cpus > initial_global)
	{
		Nombre_cpus = initial_global;
	}
	
	if(initial_global !=0)
	{
		taux= (Nombre_cpus*100) / initial_global;
	}
	std::cout<<"Le taux "<<taux<<" avec total "<< total_global<<" et initial "<< initial_global<<"\n";
	
	Adapt_borns(taux);
	
	return (initial_global-Nombre_cpus);
}




int calcul_free_cores()
{
	int taux;
	int initial_global=0;
	int total_global=0;
	string commande1,commande2;
	
	char buffer1[1024];
	char buffer2[1024];
	
	int nb_free_cores;
	
	for(int i=0;i<UsedMachine.size();i++)
	{
		initial_global += UsedMachine[i].initial;
		snprintf(buffer1, sizeof(buffer1), "ssh %s 'ps -fux | grep wait | wc -l > fichier_info.txt' ", UsedMachine[i].name.c_str());
		std::cout<<"** "<<buffer1<<"\n";
		system(buffer1);
		
		snprintf(buffer2, sizeof(buffer2), "scp %s:fichier_info.txt . ", UsedMachine[i].name.c_str());
		std::cout<<"** "<<buffer2<<"\n";
		system(buffer2);
			
		//~ commande1 = "ssh "+ UsedMachine[i].name + "  ";
		//~ std::cout<<"*** "<<commande1<<" avec "<<  UsedMachine[i].name.c_str() <<"\n";
		//~ system(commande1.c_str());
		//~ commande2= "scp "+ UsedMachine[i].name + ":fichier_info.txt .";
		//~ std::cout<<"*** "<<commande2<<"\n";
		//~ system(commande2.c_str());
		ifstream fichier("fichier_info.txt", ios::in);
		if(fichier)  
		{
			string ligne;
			while(getline(fichier, ligne))
			{
				nb_free_cores=atoi(ligne.c_str());
			}
		}
		
		if(nb_free_cores < 4)
		{
			nb_free_cores = 0;
		}else
		{
			nb_free_cores = nb_free_cores -3;
		}
		
		
		if(nb_free_cores >UsedMachine[i].initial)
		{
			nb_free_cores = UsedMachine[i].initial;
		}
		total_global += nb_free_cores;
		UsedMachine[i].total = UsedMachine[i].initial - nb_free_cores;
	}
	if(initial_global !=0)
	{
		taux= (total_global*100) / initial_global;
	}
	std::cout<<"Le taux "<<taux<<" avec total "<< total_global<<" et initial "<< initial_global<<"\n";
	
	//~ if(taux >=85)
	//~ {
		//~ add_machine();
	//~ }else
	//~ {
		//~ if(taux <=15)
		//~ {
			//~ remove_machine();
		//~ }
	//~ }
	
	
	return (initial_global-total_global);
}

int CalculP(float priority_cores, int type, std::vector<int> &id_cluster, int nb_restart)
{
	int free_cores=0;
	float total_priority=0;
	int nb_cores_first=1;
	int best_id_cluster;
	int best_p_cores;
	
	int id_machine;
	std::list<NodeT>::iterator it;

	//~ free_cores =calcul_free_cores();
	free_cores =CPUs_2();
	
	if(free_cores > 0 )
	{
		it= ListTaches.begin();
		while (it != ListTaches.end()) {
			total_priority = total_priority + it->priority_cores;
			it++;
		}
			
		nb_cores_first=(priority_cores*free_cores)/total_priority;
		
		//~ std::cout<<"JE SUIS AVEC FORMUL "<<nb_cores_first<<" type "<<type<<" Min "<<MinCores[type]<<" Max "<<MaxCores[type]<<"\n";
		//~ std::cout<<" Priority cores "<<priority_cores<<" free "<<free_cores<<" total "<<total_priority<<"\n";
		
		if(nb_cores_first>MaxCores[type])
		{
			nb_cores_first=MaxCores[type];
		}else
		{
			if(nb_cores_first< MinCores[type])
			{
				nb_cores_first=MinCores[type];
			}
		}

		std::cout<<"LE NOMBRE DE CORES "<<nb_cores_first<<" et free "<< free_cores <<"\n";
		
		bool stop=false;
		///Calculer la plus petite difference
		int diff_min=INT_MAX;
		int min_container=INT_MAX;
		//~ int diff_min=INT_MIN;
		int diff;
		id_machine=-1;
		
		
		
		//~ for(int i=0;i< UsedMachine.size(); i++)
		//~ {
			//~ diff=UsedMachine[i].total-nb_cores_first;
			
			//~ if (diff >=0 && UsedMachine[i].containers < min_container)
			//~ {
				//~ min_container=UsedMachine[i].containers;
				//~ id_machine=i;
				//~ id_cluster=i;
			//~ }
		//~ }
		
		for(int i=0;i< UsedMachine.size(); i++)
		{
			diff=UsedMachine[i].total-nb_cores_first;
			
			if (diff >=0 && id_cluster.size()< nb_restart)
			{
				id_machine=i;
				id_cluster.push_back(i);
			}
		}
		
		std::cout<<"Le nombre de restart "<<id_cluster.size() <<" et "<<nb_restart<<"\n";

		if(id_cluster.size() != nb_restart)
		{
			return 0;
		}else
		{
			if(id_machine != -1)
			{
				return nb_cores_first;
			
			}else
			{
				return 0;
			}	
		}
		
	}else
	{
		return -1; //No free cores
	}	
}
int calcul_energy()
{
	int en=0;
	for(int i=0;i<UsedMachine.size();i++)
	{
		en =en + 1 + (UsedMachine[i].initial-UsedMachine[i].total);
		
	}
	
	for(int i=0;i<PendingMachine.size();i++)
	{
		en =en + 1 + (PendingMachine[i].initial-PendingMachine[i].total);
		
	}
	int total=UsedMachine.size()+PendingMachine.size();
	
	if( total-Copie_Used_Machine>0)
	{
		//~ std::cout<<"\n\n\n Je suis la avec "<< total-Copie_Used_Machine <<" et "<< total<<" copie "<<Copie_Used_Machine<<"\n\n\n\n";
		en= en+ (total-Copie_Used_Machine)*5;
		Copie_Used_Machine=total;
	}
	
	if( total-Copie_Used_Machine<0)
	{
		//~ std::cout<<"\n\n\n Je suis la avec "<< Copie_Used_Machine-total <<" et "<< total<<" copie "<<Copie_Used_Machine<<"\n\n\n\n";
		en= en+ (Copie_Used_Machine-total)*5;
		Copie_Used_Machine=total;
	}
	return en;
	
}


bool my_compare_base (NodeT a, NodeT b)
{
    return a.priority_time > b.priority_time;
}



bool Nb_exist_machine(int type)
{
	int nb_min=MinCores[type];
	std::cout<<"Le nb "<<nb_min<<"\n";
	
	for(int i=0;i<UsedMachine.size();i++)
	{
		if(UsedMachine[i].total >= nb_min)
		{
			return true;
		}
	}
	std::cout<<"RETURN FALSE\n";
	return false;
}

bool check_machines(std::vector<int> id_cluster, int nb_p)
{
	for(int i=0;i<id_cluster.size();i++)
	{
		std::cout<<"Je suis avec "<<UsedMachine[id_cluster[i]].VNotUsedCpus.size()<< " et "<<nb_p<<"\n";
		if(UsedMachine[id_cluster[i]].VNotUsedCpus.size() < nb_p)
		{
			return false;
		}
	}
	return true;
	
}

void Allocation()
{
	std::list<NodeT>::iterator it;
	int nb_p=0;
	int nb_j=0;
	float t1;
	double t_fin;
	int cmpt;				
	int cmpt_size;
	bool stop_while=false;
	string commande1;
	std::vector<int> list_cpus;
	std::vector<int> id_cluster;
	char buffer[1024];
	char buffer2[1024];
	char buffer3[1024];
	string buffer_cpus="";

	
	ofstream fichier_output("output.txt", ios::out | ios::trunc);  
	if(!fichier_output)
	{
		std::cout<<"Problem with output file\n";
		exit(0);
	}
	
	std::cout<<"Start Allocation Function\n";
	std::cout<<"Used Machines:\n";
	
	for(int i=0;i<UsedMachine.size();i++)
	{
		std::cout<<" Name "<<UsedMachine[i].name<<" Initial number of cores "<< UsedMachine[i].initial <<" Total number of cores "<< UsedMachine[i].total<<"\n";
	}
			
	while(nb_j<NB_Jobs-1)
	{
		std::cout<<"* Containers in the queue "<< ListTaches.size()<<" I am container "<< nb_j<<" Total containers "<< NB_Jobs<<"\n";	
		while(ListTaches.empty())
		{
			sleep(1);
		}
			
		/*Trie la liste des taches selon la priority et effacer les job*/
		//~ effacer();
		//~ Promethee();
		ListTaches.sort(my_compare_base);
			
		it = ListTaches.begin();
		cmpt_size=ListTaches.size();
		cmpt=0;
		stop_while=false;
		while (it != ListTaches.end() && stop_while==false && cmpt < cmpt_size) 
		{
		id_cluster.clear();		
		std::cout<<"Priority "<<it->priority_restart<<"\n";
		nb_p=CalculP(it->priority_cores,it->type,id_cluster,it->priority_restart);
		std::cout<<"NB_Restart est "<<id_cluster.size()<<"\n";
		
		std::cout<<"Je suis apres le calcul de P "<< nb_p <<" avec type "<< it->type <<"\n";
		if( nb_p !=0 && nb_p != -1 && check_machines(id_cluster,nb_p) == true )
		{
			Simul_iteration[it->type]+=1;
			Simul_cores[it->type]+=nb_p;
			t1=float (clock() - t0) / CLOCKS_PER_SEC;
			gettimeofday(&tv2, NULL);
			t_fin= (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
			//~ std::cout<<"Avant "<<Simul_wait[it->type]<<" et t1* "<< t1<<" t0 "<< t0<<"start "<< it->start <<"\n";
			Simul_wait[it->type] += t_fin - it->start;
			Simul_type[it->type] +=nb_p;
					
			if(it->type==0)
			{
				Simul_cores_type0.push_back(nb_p);
			}
			if(it->type==1)
			{
				Simul_cores_type1.push_back(nb_p);
			}
			if(it->type==2)
			{
				Simul_cores_type2.push_back(nb_p);
			}
					
			nb_j++;
			
			for(int i=0;i<3;i++)
			{
				if(it->type == i)
				{
					fichier_output<<"1 ";
				}else
				{
					fichier_output<<"0 ";
				}
			}
			fichier_output<< nb_p <<"\n";
			VContainers[nb_j]= nb_p;


			for(int k=0;k<id_cluster.size();k++)
			{
				list_cpus.clear();
				for(int i=0;i<nb_p;i++)
				{
					list_cpus.push_back(UsedMachine[id_cluster[k]].VNotUsedCpus[i]);	
				}
			
				string buffer_cpus="";
				///Ecrire dans le fichier output
				//~ fichier_output<<"Container "<<nb_j<<" CPU "<<nb_p<<" Time "<<it->seq_timeT<<" Machine "<<UsedMachine[id_cluster].name.c_str()<<"\n";
				//~ fichier_output<<"CPUs:\n";
			
				for(int i=0;i<list_cpus.size();i++)
				{
					stringstream ss;
					//~ fichier_output<<list_cpus[i]<<"\n";
					ss << list_cpus[i];
					buffer_cpus +=ss.str() + "," ;
				}
				snprintf(buffer, sizeof(buffer), "ssh  %s 'lxc-create -t ubuntu -n %d  && sleep 1 && lxc-start -n %d -d && lxc-cgroup -n %d cpuset.cpus %s'",VConfig[id_cluster[k]].name.c_str(),nb_j,nb_j,nb_j,buffer_cpus.c_str());
				std::cout<<"** "<<buffer<<"\n";
				system(buffer);
				
				snprintf(buffer2, sizeof(buffer2), "ssh  %s 'sleep %f && lxc-stop -n %d && lxc-destroy -n %d ' &",VConfig[id_cluster[k]].name.c_str(),it->seq_timeT/nb_p,nb_j,nb_j);
				system(buffer2);
			}
			
				
				
			//Efacer le node it et sortir de la boucle
			it=ListTaches.erase(it);
			stop_while=true;
			break;
					
		}else
		{	//Il n y a pas des coeurs dispos			
			if(nb_p == -1)
			{
				usleep(500);
			}
			if(nb_p == 0)
			{						
				std::cout<<"Je suis ici avec 0 \n";
				if(it->stop_node < 10)
				{
					it->stop_node ++;
					it++;
					if(it == ListTaches.end())
					{
						stop_while=true;
					}
				}else
				{
					std::cout<<"ATTENTION JE SUIS BLOQUE \n";
					//~ while(Nb_exist_machine (it->type) == false)
					//~ {
						//~ usleep(500);
					//~ }
				}
			}
		}
		cmpt++;
		}
	}
	
	bool stop_dernier=false;
	it = ListTaches.begin();
	
	while (stop_dernier==false)
	{
		id_cluster.clear();
		nb_p=CalculP(it->priority_cores,it->type,id_cluster,it->priority_restart);
		
		if( nb_p !=0 && nb_p != -1 && check_machines(id_cluster,nb_p) == true)
		{
				
			Simul_iteration[it->type]+=1;
			Simul_cores[it->type]+=nb_p;
			t1=float (clock() - t0) / CLOCKS_PER_SEC;
			gettimeofday(&tv2, NULL);
			t_fin= (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
			//~ std::cout<<"Avant "<<Simul_wait[it->type]<<" et t1* "<< t1<<" t0 "<< t0<<"start "<< it->start <<"\n";
			Simul_wait[it->type] += t_fin - it->start;
			Simul_type[it->type] +=nb_p;
					
			if(it->type==0)
			{
				Simul_cores_type0.push_back(nb_p);
			}
			if(it->type==1)
			{
				Simul_cores_type1.push_back(nb_p);
			}
			if(it->type==2)
			{
				Simul_cores_type2.push_back(nb_p);
			}
					
			nb_j++;
			for(int i=0;i<3;i++)
			{
				if(it->type == i)
				{
					fichier_output<<"1 ";
				}else
				{
					fichier_output<<"0 ";
				}
			}
			fichier_output<< nb_p <<"\n";
			
			///lxc-create -t ubuntu -n tt && lxc-start -n tt -d && sleep 4 && lxc-stop -n tt && lxc-destroy -n tt
			VContainers[nb_j] = nb_p;	
			
			
			
			for(int k=0;k<id_cluster.size();k++)
			{
				
				list_cpus.clear();
				for(int i=0;i<nb_p;i++)
				{
					list_cpus.push_back(UsedMachine[id_cluster[k]].VNotUsedCpus[i]);	
				}
				string buffer_cpus="";

				///Ecrire dans le fichier output
				//~ fichier_output<<"Container "<<nb_j<<" CPU "<<nb_p<<" Time "<<it->seq_timeT<<" Machine "<<UsedMachine[id_cluster].name.c_str()<<"\n";
				//~ fichier_output<<"CPUs:\n";
			
				for(int i=0;i<list_cpus.size();i++)
				{
					stringstream ss;
					//~ fichier_output<<list_cpus[i]<<"\n";
					ss << list_cpus[i];
					buffer_cpus +=ss.str() + "," ;
				}
			
				snprintf(buffer, sizeof(buffer), "ssh  %s 'lxc-create -t ubuntu -n %d  && sleep 1 && lxc-start -n %d -d && lxc-cgroup -n %d cpuset.cpus %s'",VConfig[id_cluster[k]].name.c_str(),nb_j,nb_j,nb_j,buffer_cpus.c_str());
				std::cout<<"** "<<buffer<<"\n";
				system(buffer);
				
				
				snprintf(buffer2, sizeof(buffer2), "ssh  %s 'sleep %f && lxc-stop -n %d && lxc-destroy -n %d'",VConfig[id_cluster[k]].name.c_str(),it->seq_timeT/nb_p,nb_j,nb_j);
				system(buffer2);
			}	
				
			stop_dernier=true;	
			stop_work=true;	
		}
	}
	
	fichier_output.close();
	
	std::cout<<"Fin traitement \n";
	
}


/// Promethee functions

bool my_compare (NodeT a, NodeT b)
{
    return a.Promethee > b.Promethee;
}

void Promethee()
{
	int nb_requests=ListTaches.size();
	if(nb_requests>1)
	{
	std::vector<NodePromethee> ListNodes;
	
	std::list<NodeT>::iterator it;
	it= ListTaches.begin();
	
	for(int i=0;i<nb_requests;i++)
	{
		NodePromethee NP;
		NP.priority_cores=it->priority_cores;
		NP.priority_time=it->priority_time;
		ListNodes.push_back(NP);
		it++;
	}
	
	
	SPromethee **tab;
	tab = (SPromethee**) malloc(nb_requests*sizeof(SPromethee*));
	
	for(int i=0;i<nb_requests;i++)
	{
		tab[i]=(SPromethee * ) malloc(nb_requests*sizeof(SPromethee));
	}
	std::vector<float> F;
	
	//Comparaison par paire
	for(int i=0;i<nb_requests;i++)
	{
		for(int j=0;j<nb_requests;j++)
		{
			tab[i][j].D[0]=ListNodes[i].priority_cores-ListNodes[j].priority_cores;
			tab[i][j].D[1]=ListNodes[i].priority_time-ListNodes[j].priority_time;
		}
	}
	//Degré de préférence
	float P0,P1;
	for(int i=0;i<nb_requests;i++)
	{
		for(int j=0;j<nb_requests;j++)
		{
			if(tab[i][j].D[0] >0)
			{
				P0=1;
			}
			else
			{
				P0=0;
			}
			
			if(tab[i][j].D[1] >0)
			{
				P1=1;
			}
			else
			{
				P1=0;
			}
			tab[i][j].P=P0+ (2* P1);
			
		}
	}
	//Flux de préférence multicritère
	float fpositive=0,fnegative=0;
	for(int i=0;i<nb_requests;i++)
	{
		for(int j=0;j<nb_requests;j++)
		{
			fpositive +=tab[i][j].P;
		}
		
		F.push_back(fpositive/(nb_requests-1));
		fpositive=0;
	}
	
	//Fnegative
	for(int i=0;i<nb_requests;i++)
	{
		for(int j=0;j<nb_requests;j++)
		{
			fnegative +=tab[j][i].P;
		}
		
		F[i] = F[i] - (fnegative/(nb_requests-1));
		fnegative=0;
	}
	
	//~ //Affichage
	//~ for(int i=0;i<nb_requests;i++)
	//~ {
		//~ for(int j=0;j<nb_requests;j++)
		//~ {
			//~ printf(" %f - %f - %f \n",tab[i][j].D[0],tab[i][j].D[1], tab[i][j].P);
		//~ }
	//~ }
	//~ std::cout<<"Je suis ici 1\n";
	it= ListTaches.begin();
	for(int i=0;i<nb_requests;i++)
	{
		it->Promethee=F[i];
		//~ printf("F: %f \n",it->Promethee);
		it++;
	}
	
	ListTaches.sort(my_compare);
	
	for(int i=0 ; i < nb_requests ; i++){
		free(tab[i]);
	}
	free(tab); 
	}
}

/// Main Part ///

void *submission_function (void * arg)
{
	if(Submission_id==0)
	{
		submitions_BD(Submission_id_BD);
	}else
	{
		if(Submission_id == 1)
		{
			submissions_same();
		}else
		{
			submitions_pas();
		}
	}
  pthread_exit (0);
}
void *allocation_function (void * arg)
{
  Allocation();
  pthread_exit (0);
}
void *simulation_function (void * arg)
{
  simulation();
  pthread_exit (0);
}

void Function_help()
{
	
	std::cout<<"./allocation -Submission [arg] -Frequency [arg] -Time [arg] -NB_Containers [arg] \n";
	std::cout<<"-Submission : 0 if database as prezi, 1 if submission at the same time, 2 if submission with a fixed frequency .\n";
	std::cout<<"-BD : 0 BD Prezi, 1 BD Goggle trace.\n";
	std::cout<<"-Frequency : the frequence of submission \n";
	std::cout<<"-Time : the sequential life time \n";
	std::cout<<"-NB_Containers : the number of submited containers \n";
	std::cout<<"* If -Submission 0 : Dont need others arguments \n";
	std::cout<<"* If -Submission 1 : Dont need the Frequency argument \n";
	std::cout<<"Example : ./allocation -Submission 1 -Frequency 30 -Time 1500 -NB_Containers 60 \n";
}
bool Parse(int argc, char *argv[])
{
	
	std::cout<<"Je suis dans parse \n";
	string Submission="-Submission";
	string Submission_BD="-BD";
	string Time="-Time";
	string NB_Containers="-NB_Containers";
	string Frequency="-Frequency";
	
	
	
	
	for(int i =1;i<argc;i++)
	{
		if(strcmp(argv[i], Submission.c_str()) == 0)
		{
			Submission_id=atoi(argv[i+1]);
		}
		
		if(strcmp(argv[i], Submission_BD.c_str()) == 0)
		{
			Submission_id_BD=atoi(argv[i+1]);
		}
		
		
		if(strcmp(argv[i], Time.c_str()) == 0)
		{
			Sequential_life_time=atoi(argv[i+1]);
		}
		
		if(strcmp(argv[i], NB_Containers.c_str()) == 0)
		{
			NB_Jobs=atoi(argv[i+1]);
		}
		
		if(strcmp(argv[i], Frequency.c_str()) == 0)
		{
			Frequency_number=atoi(argv[i+1]);
		}
		
	}
	
	if(Submission_id == -1 || Submission_id >2)
	{
		return false;
	}else
	{
		if(Submission_id == 0)
		{
			Submition_BD = true;
			std::cout<<"Submission "<<Submission_id<<" BD "<<Submission_id_BD<<"\n";
			return true;
		}
		else
		{
			if(Submission_id == 1)
			{
				if(Sequential_life_time != 0 && NB_Jobs != 0)
				{
					std::cout<<"Submission "<<Submission_id<<" Time "<<Sequential_life_time<<" NB_Jobs"<<NB_Jobs<<"\n";
					return true;
				}else
				{
					return false;
				}
			}else
			{
				if(Submission_id == 2)
				{
					if(Sequential_life_time != 0 && NB_Jobs != 0 && Frequency_number !=0)
					{
						std::cout<<"Submission "<<Submission_id<<" Time "<<Sequential_life_time<<" NB_Jobs"<<NB_Jobs<<" Frequency "<<Frequency_number<<"\n";
						return true;
					}else
					{
						return false;
					}
				}else
				{
					return false;
				}
			}
		}
		
		
		
		
		
		
		
	
		
		
	}
	
	
	
	
}

int main(int argc, char *argv[])
{
	string Help="-Help", help="-help", H="-H",h="-h";
	
	
	bool commande=false;
		
	
		if(argc == 1)
		{
			std::cout<<"You must consult -help \n";
			exit(0);
			
		}else
		{
			if(argc == 2)
			{
				if((strcmp(argv[1], Help.c_str()) == 0) || (strcmp(argv[1], help.c_str()) == 0) || (strcmp(argv[1], H.c_str()) == 0) || (strcmp(argv[1], h.c_str()) == 0))
				{ 
					Function_help();
					exit(0);
				}else
				{
					std::cout<<"You must consult -help \n";
					exit(0);
				}
			
			}else
			{
				std::cout<<"Je suis ici \n";
				if(Parse(argc,argv)==false)
				{
					std::cout<<"You must consult -help \n";
					exit(0);
				}
			}	
		}		
			
	std::cout<<"Je suis fin \n";
	
	
	
  pthread_t th1, th2,th3;
  void *ret;

   Configuration();

  if (pthread_create (&th1, NULL, submission_function, NULL) < 0) {
    fprintf (stderr, "pthread_create for submission function error\n");
    exit (1);
  }

  if (pthread_create (&th2, NULL, allocation_function, NULL) < 0) {
    fprintf (stderr, "pthread_create for allocation function error\n");
    exit (1);
  }
  
   if (pthread_create (&th3, NULL, simulation_function, NULL) < 0) {
    fprintf (stderr, "pthread_create for simulation function error\n");
    exit (1);
  }

  (void)pthread_join (th1, &ret);
  (void)pthread_join (th2, &ret);
  (void)pthread_join (th3, &ret);
	
   return 0;
}
