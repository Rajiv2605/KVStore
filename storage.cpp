#include "storage.hpp"
#include <iostream>
#include <sstream>
#include <string>

void Storage::reader_lock()
{
    unique_lock<mutex> lock(m);
    cv.wait(lock, [&]{ return !writers; });
    readers++;
    lock.unlock();
}

void Storage::reader_unlock()
{
    unique_lock<mutex> lock(m);
    readers--;
    if(readers <= 0)
        cv.notify_all();
    lock.unlock();
}

void Storage::writer_lock()
{
    unique_lock<mutex> lock(m);
    cv.wait(lock, [&]{ return !writers && (readers <= 0); } );
    writers = true;
    lock.unlock();
}

void Storage::writer_unlock()
{
    unique_lock<mutex> lock(m);
    writers = false;
    cv.notify_all();
    lock.unlock();
}

string Storage::handle_get(string key)
{
    int idx = check_hit(key);
    if(idx != -1)
    {
        string value = LLC[idx].value;
        // respond here
        cout<<"Cache hit: "<<value<<endl;
        return value;
    }

    // binary search on keys
    int kidx = key[key.size()-1];
    kidx = kidx%10;
    auto pos = keys[kidx].find(key);
    if(pos==keys[kidx].end())
    {
        cout<<"KEY NOT FOUND!"<<endl;
        return "ERROR";
    }

    int offs = table[kidx][key];
    f_db[kidx].seekp(offs, ios::beg);
    string line;
    getline(f_db[kidx], line);
    stringstream getVal(line);
    string k, v;
    getVal>>k;
    getVal>>v;
    cout<<"value: "<<v<<endl;
    // respond here
    fill_cache(k, v);
    return v;
}

void Storage::handle_put(string key, string value)
{
    int idx = check_hit(key);
    for(int i=0; i<cache_set; i++)
        if(LLC[i].key==key)
        {
            idx = i;
            break;
        }
    if(idx > -1)
    {
        LLC[idx].value = value;
        LLC[idx].valid = 1;
    }

    int kidx = key[key.size()-1];
    kidx = kidx%10;
    string line;
    f_db[kidx].seekg(0, ios::beg);
    bool written = false;
    // bool isEmpty = f_db.peek() == EOF;
    string newl = key + " " + value;
    if(isEmpty[kidx])
    {
        f_db[kidx].seekg(0, ios::beg);
        f_db[kidx]<<newl<<endl;
        table[kidx][key] = 0;
        keys[kidx].insert(key);
        linesizes[kidx][key] = newl.size();
        isEmpty[kidx] = false;
        return;
    }

    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);

    // check if the key is already present in the DB
    bool isPresent = false;
    for(auto i=keys[kidx].begin(); i!=keys[kidx].end(); i++)
        if(*i==key)
        {
            isPresent = true;
            break;
        }

    while(getline(f_db[kidx], line))
    {
        stringstream getVals(line);
        string k;
        getVals>>k;
        if(k < key)
        {
            newdb<<line<<endl;
        }
        else if(k > key)
        {
            if(!written)
            {
                newdb<<newl<<endl;
                written = true;

                // calculate offset for current line here
                if(!isPresent)
                {   
                    // cout<<"Inserting new: "<<K<<endl;
                    string maxkey = "";
                    for(auto x = keys[kidx].begin(); x != keys[kidx].end(); x++)
                    {
                        if(*x > key)
                            break;
                        maxkey = *x;
                    }
                    if(maxkey == "")
                        table[kidx][key] = 0;
                    else
                        table[kidx][key] = table[kidx][maxkey] + linesizes[kidx][maxkey] + 1;
                }
            }

            newdb<<line<<endl;
            // update offsets here with newline.sze() + 1
            if(!isPresent)
                table[kidx][k] += newl.size() + 1;
        }
    }

    if(!written)
    {
        newdb<<newl<<endl;
        // calculate offset for current line here
        if(!isPresent)
        {
            string maxkey;
            for(auto x = keys[kidx].begin(); x != keys[kidx].end(); x++)
            {
                if(*x > key)
                    break;
                maxkey = *x;
            }
            table[kidx][key] = table[kidx][maxkey] + linesizes[kidx][maxkey] + 1;
        }
    }
    keys[kidx].insert(key);
    linesizes[kidx][key] = newl.size();
    newdb.close();
    f_db[kidx].close();
    string dbfname = "keydb" + to_string(kidx) + ".txt";
    remove(dbfname.c_str());
    rename("temp.txt", dbfname.c_str());
    f_db[kidx].open(dbfname, ios::out | ios::app | ios::in);
}

string Storage::handle_delete(string key)
{
    int idx = check_hit(key);
    if(idx > -1)
        LLC[idx].valid = 0;

    bool found = false;
    int kidx = key[key.size()-1];
    kidx = kidx%10;
    for(auto i=keys[kidx].begin(); i!=keys[kidx].end(); i++)
        if(*i==key)
        {
            found = true;
            break;
        }
    if(!found)
    {
        cout<<"KEY NOT FOUND!"<<endl;
        return "ERROR";
    }

    // remove entries
    table[kidx].erase(key);
    int lsz = linesizes[kidx][key]+1;
    linesizes[kidx].erase(key);
    set<string>::iterator i;
    for(i=keys[kidx].begin(); i!=keys[kidx].end(); i++)
        if(*i==key)
            break;
    keys[kidx].erase(i);
    for(map<string, uint64_t>::iterator i=table[kidx].begin(); i!=table[kidx].end(); i++)
    {
        if(i->first > key)
            i->second -= lsz;
    }

    f_db[kidx].seekg(0, ios::beg);
    string line;
    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);
    while(getline(f_db[kidx], line))
    {
        stringstream getVals(line);
        string k;
        getVals>>k;
        if(k != key)
            newdb<<line<<endl;
    }

    newdb.close();
    f_db[kidx].close();
    string dbfname = "keydb" + to_string(kidx) + ".txt";
    remove(dbfname.c_str());
    rename("temp.txt", dbfname.c_str());
    f_db[kidx].open(dbfname, ios::out | ios::app | ios::in);
    return "SUCCESS";
}

// int main()
// {
//     // WRITE PARSER
    
//     // sample received string
//     string s, key, method;

//     Storage sr;
//     sr.handle_put("5", "500");
//     sr.handle_put("4", "400");
//     sr.handle_put("3", "300");
//     sr.handle_put("2", "200");

//     sr.handle_delete("3");
//     // sr.handle_put("2", "100");
//     // sr.handle_delete("2");
//     // sr.handle_put("2", "200");

//     // // sr.handle_delete("4");
//     // sr.handle_get("1");
//     // sr.handle_get("3");
//     sr.handle_get("2");
//     // sr.handle_get("4");
//     // sr.handle_get("5");
//     // sr.handle_delete("13");

//     for(auto i=sr.table.begin(); i!=sr.table.end(); i++)
//         cout<<i->first<<" "<<i->second<<endl;

//     return 0;
// }