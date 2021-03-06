#include <set>
#include <map>
#include <mutex>
#include <vector>
#include <fstream>
#include <cstring>
#include <sstream>
#include <condition_variable>
#include <iostream>
using namespace std;

class Storage
{
public:                   // later move variables to private (public for ease of testing)
    vector<bool> bitmap;  // keeps track of empty lines to handle the PUT request
    vector<fstream> f_db; // handles the key-value db in persistent storage
    // fstream f_log;          // handles the log file
    vector<map<string, uint64_t>> table; // stores offset for keys
    vector<map<string, int>> linesizes;  // stores line size for keys
    vector<set<string>> keys;            // stores keys
    bool isEmpty[10];
    ifstream f_config;

    // cache block structure
    struct cache_block
    {
        string key;
        string value;
        int valid;
        int lru;
        int lfu;
    };

    // lock related
    int id;
    int readers = 0;
    int writers = false;
    mutex m;
    condition_variable cv;

    string policy;
    int cache_set; //cache_size/key_value_size;
    struct cache_block LLC[/*cache_set*/ 64];

    Storage()
    {
        // init
        f_db = vector<fstream>(10);
        f_config.open("config.txt");
        string line;
        getline(f_config, line);
        getline(f_config, policy);
        getline(f_config, line);
        stringstream ss(line);

        ss >> cache_set;
        cout << "# of cache sets: " << cache_set << endl;
        cout << "cache replacement policy: " << policy << endl;
        // f_log.open("log.txt", ios::out | ios::app | ios::in);

        for (int i = 0; i < cache_set; i++)
        {
            LLC[i].key = '\0';
            LLC[i].value = '\0';
            LLC[i].lru = -1;
            LLC[i].lfu = 0;
        }

        table = vector<map<string, uint64_t>>(10);
        linesizes = vector<map<string, int>>(10);
        keys = vector<set<string>>(10);
        memset(isEmpty, true, sizeof(isEmpty));
    }

    ~Storage()
    {
        for (int i = 0; i < 10; i++)
            f_db[i].close();
        //   f_log.close();
    }

    void set_server_id(int sid)
    {
        id = sid;
    }

    void create_db()
    {
        for (int i = 0; i < 10; i++)
        {
            string name = "keydb" + to_string(id) + "_" + to_string(i) + ".txt";
            f_db[i].open(name, ios::out | ios::app | ios::in);
        }
    }

    // commit logs for the given key
    // void commit_logs(string key);

    // appending to db
    // void write_put(string key, string value, int index);

    // deleting from DB
    // void write_del(string key, int index);

    // Readers-Writer lock
    void reader_lock();
    void reader_unlock();
    void writer_lock();
    void writer_unlock();

    // declare methods to handle clients
    // void receive();
    // void respond();

    // declare methods to service the requests and respond
    string handle_get(string key);
    void handle_put(string key, string value);
    string handle_delete(string key);

    //Cache
    int check_hit(string key);
    int llc_lru_find_victim();
    int llc_lfu_find_victim();
    void llc_lru_update(int index);
    void llc_lfu_update(int index);
    void fill_cache(string key, string value);
    void print_cache();
};
