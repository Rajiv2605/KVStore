#include <set>
#include <map>
#include <vector>
#include <fstream>
#include <cstring>

using namespace std;

class Server
{
  public: // later move variables to private (public for ease of testing)
    vector<bool> bitmap;    // keeps track of empty lines to handle the PUT request
    vector<fstream> f_db;           // handles the key-value db in persistent storage
    // fstream f_log;          // handles the log file
    vector<map<int, uint64_t>> table;   // stores offset for keys
    vector<map<int, int>> linesizes;    // stores line size for keys
    vector<set<int>> keys;              // stores keys
    bool isEmpty[10];

    // cache block structure
    struct cache_block{
	string key;
	string value;
	int valid;
	int lru;
	int lfu;
	};

    int lru =0,lfu=1;
    int cache_set=4;    //cache_size/key_value_size;
    struct cache_block LLC[/*cache_set*/ 64];

    Server()
    {
        // init
        f_db = vector<fstream>(10);
        for(int i=0; i<10; i++)
        {
            string name = "keydb" + to_string(i) + ".txt";
            f_db[i].open(name, ios::out | ios::app | ios::in);
        }
        // f_log.open("log.txt", ios::out | ios::app | ios::in);

    	for(int i=0;i<cache_set;i++)
        {
            LLC[i].key='\0';
            LLC[i].value='\0';
            LLC[i].lru=-1;
            LLC[i].lfu=0;
        }

        table = vector<map<int, uint64_t>>(10);
        linesizes = vector<map<int, int>>(10);
        keys = vector<set<int>>(10);
        memset(isEmpty, true, sizeof(isEmpty));
    }

    ~Server()
    {
        for(int i=0; i<10; i++)
            f_db[i].close();
    //   f_log.close();
    }

    // commit logs for the given key
    // void commit_logs(string key);

    // appending to db
    // void write_put(string key, string value, int index);
    
    // deleting from DB
    // void write_del(string key, int index);

    // Readers-Writer lock for logfile handling
    // void db_lock();
    // void db_unlock();
    // void log_lock();
    // void log_unlock();

    // declare methods to handle clients
    // void receive();
    // void respond();

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
