#include "server.hpp"
#include <iostream>
#include <sstream>
#include <string>

void Server::db_lock()
{

}

void Server::db_unlock()
{

}

void Server::log_lock()
{

}

void Server::log_unlock()
{

}

void Server::write_put(string key, string value, int index)
{
    cout<<"Calling write_put for "<<key<<" "<<value<<" "<<index<<endl;
    f_db.seekg(0, ios::beg);
    string dbline;
    int lidx=0;
    while(f_db)
    {
        cout<<"Inside"<<endl;
        // go to line where insertion has to be done
        if(lidx < index)
        {
            getline(f_db, dbline);
            lidx++;
            continue;
        }
            cout<<"inserting at: "<<lidx<<endl;
            // write in db
            string wr_line;
            wr_line = key + " " + value;
            f_db<<wr_line<<endl;

            // update in cache as well
            break;
    }
}

void Server::write_del(string key, int index)
{
    f_db.seekg(0, ios::beg);
    fstream newdb;
    newdb.open("temp2.txt", ofstream::out);
    string line;
    int lidx = 0;
    while(f_db)
    {
        getline(f_db, line);
        if(lidx != index)
        {
            newdb<<line<<endl;
            lidx++;
        }
    }
    bitmap[index] = false;
    f_db.close();
    newdb.close();
    remove("keydb.txt");
    rename("temp2.txt", "keydb.txt");
    f_db.open("keydb.txt", ios::out | ios::app | ios::in);
}

void Server::commit_logs(string key)
{
    cout<<"Committing logs..."<<endl;
    log_lock();
    // before reading commit all the logs
    f_log.seekg(0, ios::beg);
    string line;

    // remove log entry
    ofstream newlog;
    newlog.open("temp.txt", ofstream::out);

    while(f_log)
    {
        getline(f_log, line);
        stringstream getVal(line);
        string m, k, v;
        int i;
        getVal>>m;
        getVal>>k;
        getVal>>v;
        getVal>>i;
        if(m=="PUT")
            write_put(k, v, i);
        else
            write_del(k, i);
    }
    log_unlock();
    f_log.close();
    newlog.close();
    remove("log.txt");
    rename("temp.txt", "log.txt");
    f_log.open("log.txt", ios::out | ios::app | ios::in);
    cout<<"Committed!"<<endl;
}

// called when GET()
void Server::handle_get(string key)
{
    // commit the pending logs for the given key
    commit_logs(key);

    // check hit in cache
    // if missed go below

    if(bitmap.size()==0)
    {
        cout<<"handle_get: DB is empty!"<<endl;
        return;
    }

    f_db.seekg(0, ios::beg);

    // reading db line by line
    string line;
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
    // check hit in cache
    // if present write there
    // write through

    // check if present in db already
    // if present update here, no need of logs
    f_db.seekg(0, ios::beg);
    string dbline;
    while(f_db)
    {
        getline(f_db, dbline);
        stringstream getVal(dbline);
        string k, v;
        getVal>>k;
        getVal>>v;
        if(key==k)
        {
            string nwline = key + " " + value;
            f_db<<nwline<<endl;
            return;
        }
    }

    // f_db.close();
    // f_db.open("keydb.txt", ios::out | ios::app | ios::in);

    // continue down if PUT results in new entry
    // check bitmap and update it
    int idx;
    for(idx=0; idx<bitmap.size(); idx++)
    {
        if(!bitmap[idx])
            break;
    }

    if(idx==bitmap.size())
        bitmap.push_back(true);
    else
        bitmap[idx] = true;

    log_lock();
    // add entry in log
    f_log.seekg(0, ios::end);
    string line = "PUT " + key + " " + value + " " + to_string(idx);
    f_log<<line<<endl;
    log_unlock();
}

void Server::handle_delete(string key)
{
    // commit the pending logs for the given key
    commit_logs(key);

    if(bitmap.size()==0)
    {
        cout<<"handle_delete: DB is empty!"<<endl;
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
            log_lock();
            // add entry in log
            f_log.seekg(0, ios::end);
            string line = "DEL " + key + " " + to_string(idx);
            f_log<<line<<endl;
            log_unlock();
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

    sr.handle_put("1", "100");
    // sr.handle_put("2", "100");
    // sr.handle_put("2", "200");
    // sr.handle_put("3", "100");
    // sr.handle_get("1");
    // sr.handle_get("3");
    // sr.handle_delete("1");

    return 0;
}
