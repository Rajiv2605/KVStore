#include "server.hpp"
#include <iostream>
#include <sstream>
#include <string>

void Server::lock()
{

}

void Server::unlock()
{

}

// called when GET()
void Server::handle_get(string key)
{
    lock();
    // before reading commit all the logs
    f_log.seekg(0, ios::beg);
    while(f_log)
    {
        getline(f_log, line);
        stringstream getVal(line);
        string m, k, v, i;
        getVal>>m;
        getVal>>k;
        getVal>>v;
        getVal>>i;

        if(m=="PUT")
        {
            // find line to write at
            // write there
            // remove log entry
        }
        else if(m=="DEL")
        {
            // go to line where deletion has to be done
            // delete the line
            // remove log entry
        }
        else
        {

        }
    }
    unlock();

    string line;

    f_db.seekg(0, ios::beg);

    // reading db line by line
    while(f_db)
    {
        getline(f_db, line);
        stringstream getVal(line);
        string k, v;
        getVal>>k;
        getVal>>v;
        if(key==k)
            break;
    }

    // push k-v to cache
    // write_cache();

    // respond with the v.

}

void Server::handle_put(string key, string value)
{
    if(bitmap.size()==0)
    {
        cout<<"DB is empty!"<<endl;
        return;
    }

    // check bitmap and update it
    int idx=0;
    for(idx=0; idx<bitmap.size(); idx++)
    {
        if(!bitmap[idx])
            break;
    }

    if(idx==bitmap.size())
        bitmap.push_back(false);
    else
        bitmap[idx] = false;

    lock();
    // add entry in log
    f_log.seekg(0, ios::beg);
    string line = "PUT " + key + " " + value + " " + to_string(idx);
    f_log<<line<<endl;
    unlock();
}

void Server::handle_delete(string key)
{
    if(bitmap.size()==0)
    {
        cout<<"DB is empty!"<<endl;
        return;
    }

    int idx=0;
    string line;
    f_db.seekg(0, ios::beg);
    while(f_db)
    {
        getline(f_db, line);
        stringstream getVal(line);
        string k, v;
        getVal>>k;
        getVal>>v;
        if(key==k)
        {
            lock();
            // add entry in log
            f_log.seekg(0, ios::beg);
            string line = "DEL " + key + " " + to_string(idx);
            f_log<<line<<endl;
            unlock();
            break;
        }
        idx++;
    }

    if(idx==bitmap.size())
        cout<<"Key: "<<key<<" was not found!"<<endl;
}

int main()
{
    // WRITE PARSER
    
    // sample received string
    string s, key, method;

    Server sr;

    sr.handle_get(key);

    return 0;
}
