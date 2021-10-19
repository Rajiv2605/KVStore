#include <vector>
#include <fstream>

using namespace std;

class Server
{
  public: // later move variables to private (public for ease of testing)
    vector<bool> bitmap;    // keeps track of empty lines to handle the PUT request
    fstream f_db;           // handles the key-value db in persistent storage
    fstream f_log;          // handles the log file
    struct cache_block{
	string key;
	string value;
	int valid;
	int lru;
	int lfu;
	};
    Server()
    {
        // init
        f_db.open("keydb.txt", ios::app | ios::in);
        f_log.open("log.txt", ios::app | ios::in);
    	for(int i=0;i<cache_set;i++)
	{
		LLC[i].key='\0';
		LLC[i].value='\0';
		LLC[i].lru=-1;
		LLC[i].lfu=0;
	}
    }

    ~Server()
    {
      f_db.close();
      f_log.close();
    }

    // Readers-Writer lock for logfile handling
    void lock();
    void unlock();

    // declare methods to handle clients
    void receive();
    void respond();

    // declare methods to service the requests and respond
    void handle_get(string key);
    void handle_put(string key, string value);
    void handle_delete(string key);
    
    //Cache 
    int check_hit(string key);
    int llc_lru_find_victim();
    int llc_lfu_find_victim();
    void llc_lru_update(int index);
    void llc_lfu_update(int index);
    void fill_cache(string key,string value);
    void print_cache();
};
