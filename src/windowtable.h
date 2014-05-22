#ifndef _SENTENCE2VEC_WINDOW_TABLE_H_
#define _SENTENCE2VEC_WINDOW_TABLE_H_

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <climits>
#include <cmath>
#include "utils.h"
#include "vocab.h"
using namespace std;

namespace sent2vec {

const IndexType RAND_TABLE_SIZE = 200000;

vector<vstr>  genWindowsFromSentence(costr sent, int size)
{
    vstr words; 
    split(sent, words, " ");
    vector< vstr > windows;
    // skip sentences too short
    if(words.size() < size) 
    {
        // cout << "skip sentence:" << sent << endl;
        return windows;
    }

    for (int i=0; i<words.size() - size; i++)
    {
        vstr window; 
        window.assign(words.begin()+i, words.begin()+i+size);
        windows.push_back(window);
    }
    return windows;
}

// generate key for a window
// :parameters:
//  @words: vector of words
string genWindowKey(Vocab &vocab, vstr &words)
{
    vector<IndexType> ids;
    for(vsit wt=words.begin(); wt!=words.end(); ++wt)
    {
        IndexType id = vocab.index(*wt);
        ids.push_back(id);
    }
    return join(ids, "-");
}

vector<string> windows2WordIndexPair(Vocab &vocab, vector< vstr> windows)
{
    vstr indexs;
    for(vector< vstr >::iterator wt=windows.begin();
            wt!=windows.end(); ++wt)
    {
        vstr ids;
        for (vstr::iterator it=wt->begin();
                it!=wt->end(); ++it)
        {
            IndexType id = vocab.index(*it);
            stringstream ss;
            ss << id;
            ids.push_back(ss.str());
        }
        string joinedStr = join(ids, "-");
        indexs.push_back(joinedStr);
    }
    return indexs;
}


class WindowTable
{

public:
    // for window-count-map
    typedef map<string, IndexType> dicType ;

    // default constructor
    WindowTable():TABLE_SIZE(1e7), POWER(0.75){};

    /*
     * constructor
     *
     * :parameters:
     *  @size: size of window
     */
    WindowTable(int size, Vocab *vocab, costr path, IndexType TABLE_SIZE=1e7, float POWER=0.75): size(size), vocab(vocab), path(path), TABLE_SIZE(TABLE_SIZE), POWER(POWER){ };

    void init(int size, Vocab *vocab, costr path, IndexType TABLE_SIZE=1e7, float POWER=0.75)
    {
        this->size = size;
        this->vocab = vocab;
        this->path = path;
        // init random status
        currentRandom = 27;
        // init random table
        for(int i=0; i<RAND_TABLE_SIZE; i++) {
            IndexType randValue = rand() % TABLE_SIZE;
            rands[i] = randValue;
        }
    }

    // process a single sentence
    dicType initWinCountDicFromFile()
    {
        cout << "initWinCountDicFromFile: size:" << size << endl;

        ifstream infile(path.c_str());
        string sent;

        // window -- count map
        dicType winCountDic;
        while(getline(infile, sent))
        {
            //cout << "parsing: " << sent << endl;
            sent = trim(sent);
            vector<vstr> windows = genWindowsFromSentence(sent, size);
            vector<string> indexs = windows2WordIndexPair(*vocab, windows);
            for( vsit it=indexs.begin(); it!=indexs.end(); ++it)
            {
                dicType::iterator pos = \
                        winCountDic.find(*it);
                // do not exists
                if(pos == winCountDic.end())
                {
                    // insert an empty record
                    winCountDic.insert(
                        dicType::value_type(*it, 1));
                }
                else
                {
                    pos->second ++;
                } // end if
            } // end for
        } // end while
        return winCountDic;
    }

    const vector<IndexType> &genTable()
    {
        dicType winCountDic = initWinCountDicFromFile();
        assert(!winCountDic.empty());
        // init table
        table.resize(TABLE_SIZE, 0);
        table.reserve(TABLE_SIZE);
        // init winCountVec
        winCountVec.clear();

        IndexType vocabSize = winCountDic.size();

        float trainWindowsPowSum = 0.0;

        // winCountDic to winCountVec

        for(dicType::iterator it=winCountDic.begin();
                it!=winCountDic.end(); ++it)
        {
            trainWindowsPowSum += pow(it->second, POWER);
            winCountVec.push_back(*it);
        }
        cout << "get train windows pow sum: " << trainWindowsPowSum << endl;

        IndexType i = 0;
        
        float d1 = pow(
                (float)(winCountVec[i].second) / trainWindowsPowSum, POWER);

        for(IndexType a=0; a<TABLE_SIZE; ++a)
        {
            table[a] = i;
            if (a*1.0 / TABLE_SIZE > d1)
            {
                i++;
                d1 += pow(
                (float)(winCountVec[i].second) / trainWindowsPowSum, POWER);
                if (i >= vocabSize) i = vocabSize - 1;
            } // end if
        } // end for
        return table;
    }

    vstr getSamples(int k)
    {
        if(table.empty())
        {
            genTable();
        }
        vstr samples;
        // init random env
        //srand( (unsigned)time(NULL));

        for(int i=0; i<k; i++)
        {
            // generate a random int
            //IndexType randValue = rand() % TABLE_SIZE;
            currentRandom = rands[currentRandom % RAND_TABLE_SIZE]; 
            IndexType idx = table[currentRandom];
            string key = winCountVec[idx].first;
            samples.push_back(key);
            //cout << idx << " ";
        }
        //cout << endl;
        return samples;
    }


private:
    int size;
    Vocab *vocab;
    string path;
    const IndexType TABLE_SIZE;
    const float POWER;
    vector<IndexType> table;
    // window count map to vector
    // index(12-1221) , count
    vector< pair<string, IndexType> > winCountVec;
    IndexType rands[RAND_TABLE_SIZE];
    IndexType currentRandom;
};

//
}; // end namespace

#endif
