
/******************************************************
 *  Presage, an extensible predictive text entry system
 *  ---------------------------------------------------
 *
 *  Copyright (C) 2008  Matteo Vescovi <matteo.vescovi@yahoo.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
                                                                             *
                                                                **********(*)*/


#include "plugins/ARPAPlugin.h"


#include <sstream>
#include <algorithm>
#include <cmath>

const Variable ARPAPlugin::LOGGER     = Variable("Presage.Plugins.ARPAPlugin.LOGGER");
const Variable ARPAPlugin::ARPAFILENAME = Variable("Presage.Plugins.ARPAPlugin.ARPAFILENAME");
const Variable ARPAPlugin::VOCABFILENAME = Variable("Presage.Plugins.ARPAPlugin.VOCABFILENAME");
const Variable ARPAPlugin::TIMEOUT = Variable("Presage.Plugins.ARPAPlugin.TIMEOUT");


#define OOV "<UNK>"



ARPAPlugin::ARPAPlugin(Configuration* config, ContextTracker* ct)
    : Plugin(config,
	     ct,
             "ARPAPlugin",
             "ARPAPlugin, a plugin relying on an ARPA language model",
             "ARPAPlugin, long description." )
{
    Value value;

    try {
	value = config->get(LOGGER);
	logger << setlevel(value);
	logger << INFO << "LOGGER: " << value << endl;
    } catch (Configuration::ConfigurationException ex) {
	logger << WARN << "Caught ConfigurationException: " << ex.what() << endl;
    }

    try {
        value = config->get(VOCABFILENAME);
        logger << INFO << "VOCABFILENAME: " << value << endl;
        vocabFilename = value;

    } catch (Configuration::ConfigurationException ex) {
        logger << ERROR << "Caught fatal ConfigurationException: " << ex.what() << endl;
        throw PresageException("Unable to init " + name + " predictive plugin.");
    }

    try {
	value = config->get(ARPAFILENAME);
	logger << INFO << "ARPAFILENAME: " << value << endl;
	arpaFilename = value;

    } catch (Configuration::ConfigurationException ex) {
	logger << ERROR << "Caught fatal ConfigurationException: " << ex.what() << endl;
	throw PresageException("Unable to init " + name + " predictive plugin.");
    }

    try {
        value = config->get(TIMEOUT);
        logger << INFO << "TIMEOUT: " << value << endl;
        timeout = atoi(value.c_str());

    } catch (Configuration::ConfigurationException ex) {
        logger << ERROR << "Caught fatal ConfigurationException: " << ex.what() << endl;
        throw PresageException("Unable to init " + name + " predictive plugin.");
    }

    loadVocabulary();
    createARPATable();

}

void ARPAPlugin::loadVocabulary()
{
  std::ifstream vocabFile;
  vocabFile.open(vocabFilename.c_str());
  if(!vocabFile)
    logger << ERROR << "Error opening vocabulary file: " << vocabFilename << endl;

  assert(vocabFile);
  std::string row;
  int code = 0;
  while(std::getline(vocabFile,row))
  {
    if(row[0]=='#')
      continue;

    vocabCode[row]=code;
    vocabDecode[code]=row;

    //logger << DEBUG << "["<<row<<"] -> "<< code<<endl;

    code++;
  }

  logger << DEBUG << "Loaded "<<code<<" words from vocabulary" <<endl;

}

void ARPAPlugin::createARPATable()
{
  std::ifstream arpaFile;
  arpaFile.open(arpaFilename.c_str());

  if(!arpaFile)
    logger << ERROR << "Error opening ARPA model file: " << arpaFilename << endl;

  assert(arpaFile);
  std::string row;

  int currOrder = 0;

  unigramCount = 0;
  bigramCount  = 0;
  trigramCount = 0;

  int lineNum =0;
  bool startData = false;

  while(std::getline(arpaFile,row))
  {
    lineNum++;
    if(row.empty())
      continue;

    if(row == "\\end\\")
      break;

    if(row == "\\data\\")
    {
      startData = true;
      continue;
    }


    if( startData == true && currOrder == 0)
    {
      if( row.find("ngram 1")==0 )
      {
        unigramTot = atoi(row.substr(8).c_str());
        logger << DEBUG << "tot unigram = "<<unigramTot<<endl;
        continue;
      }

      if( row.find("ngram 2")==0)
      {
        bigramTot = atoi(row.substr(8).c_str());
        logger << DEBUG << "tot bigram = "<<bigramTot<<endl;
        continue;
      }

      if( row.find("ngram 3")==0)
      {
        trigramTot = atoi(row.substr(8).c_str());
        logger << DEBUG << "tot trigram = "<<trigramTot<<endl;
        continue;
      }
    }

    if( row == "\\1-grams:" && startData)
    {
      currOrder = 1;
      std::cerr << std::endl << "ARPA loading unigrams:" << std::endl;
      unigramProg = new ProgressBar<char>(std::cerr);
      continue;
    }

    if( row == "\\2-grams:" && startData)
    {
      currOrder = 2;
      std::cerr << std::endl << std::endl << "ARPA loading bigrams:" << std::endl;
      bigramProg = new ProgressBar<char>(std::cerr);
      continue;
    }

    if( row == "\\3-grams:" && startData)
    {
      currOrder = 3;
      std::cerr << std::endl << std::endl << "ARPA loading trigrams:" << std::endl;
      trigramProg = new ProgressBar<char>(std::cerr);
      continue;
    }

    if(currOrder == 0)
      continue;

    switch(currOrder)
    {
      case 1:   addUnigram(row);
                break;

      case 2:   addBigram(row);
                break;

      case 3:   addTrigram(row);
                break;
    }

  }

  std::cerr << std::endl << std::endl;

  logger << DEBUG << "loaded unigrams: "<< unigramCount << endl;
  logger << DEBUG << "loaded bigrams: " << bigramCount << endl;
  logger << DEBUG << "loaded trigrams: "<< trigramCount << endl;
}

void ARPAPlugin::addUnigram(std::string row)
{
  std::stringstream str(row);
  float logProb = 0;
  float logAlfa = 0;
  std::string wd1Str;

  str >> logProb;
  str >> wd1Str;
  str >> logAlfa;


  if(wd1Str != OOV )
  {
    int wd1 = vocabCode[wd1Str];

    unigramMap[wd1]= ARPAData(logProb,logAlfa);

    //logger << DEBUG << "adding unigram ["<<wd1Str<< "] -> "<<logProb<<" "<<logAlfa<<endl;
  }


  unigramCount++;

  unigramProg->update((float)unigramCount/(float)unigramTot);
}

void ARPAPlugin::addBigram(std::string row)
{
  std::stringstream str(row);
  float logProb = 0;
  float logAlfa = 0;
  std::string wd1Str;
  std::string wd2Str;

  str >> logProb;
  str >> wd1Str;
  str >> wd2Str;
  str >> logAlfa;

  if(wd1Str != OOV && wd2Str != OOV)
  {
    int wd1 = vocabCode[wd1Str];
    int wd2 = vocabCode[wd2Str];

    bigramMap[BigramKey(wd1,wd2)]=ARPAData(logProb,logAlfa);

    //logger << DEBUG << "adding bigram ["<<wd1Str<< "] ["<<wd2Str<< "] -> "<<logProb<<" "<<logAlfa<<endl;
  }

  bigramCount++;
  bigramProg->update((float)bigramCount/(float)bigramTot);
}

void ARPAPlugin::addTrigram(std::string row)
{
  std::stringstream str(row);
  float logProb = 0;

  std::string wd1Str;
  std::string wd2Str;
  std::string wd3Str;

  str >> logProb;
  str >> wd1Str;
  str >> wd2Str;
  str >> wd3Str;

  if(wd1Str != OOV && wd2Str != OOV && wd3Str != OOV)
  {
    int wd1 = vocabCode[wd1Str];
    int wd2 = vocabCode[wd2Str];
    int wd3 = vocabCode[wd3Str];

    trigramMap[TrigramKey(wd1,wd2,wd3)]=logProb;
    //logger << DEBUG << "adding trigram ["<<wd1Str<< "] ["<<wd2Str<< "] ["<<wd3Str<< "] -> "<<logProb <<endl;

  }

  trigramCount++;
  trigramProg->update((float)trigramCount/(float)trigramTot);
}


ARPAPlugin::~ARPAPlugin()
{
    delete unigramProg;
    delete bigramProg;
    delete trigramProg;
}

bool ARPAPlugin::matchesPrefixAndFilter(std::string word, std::string prefix, const char** filter ) const
{
  if(filter == 0)
    return word.find(prefix)==0;

  for(int j = 0; filter[j] != 0; j++)
  {
    std::string pattern = prefix+std::string(filter[j]);
    if(word.find(pattern)==0)
      return true;
  }

  return false;
}

Prediction ARPAPlugin::predict(const size_t max_partial_prediction_size, const char** filter) const
{
  logger << DEBUG << "predict()" << endl;
  Prediction prediction;

  int cardinality = 3;
  std::vector<std::string> tokens(cardinality);

  std::string prefix = strtolower(contextTracker->getToken(0));
  std::string wd2Str = strtolower(contextTracker->getToken(1));
  std::string wd1Str = strtolower(contextTracker->getToken(2));

  std::multimap<float,std::string,cmp> result;

  logger << DEBUG << "["<<wd1Str<<"]"<<" ["<<wd2Str<<"] "<<"["<<prefix<<"]"<<endl;

  //search for the past tokens in the vocabulary
  std::map<std::string,int>::const_iterator wd1It,wd2It;
  wd1It = vocabCode.find(wd1Str);
  wd2It = vocabCode.find(wd2Str);

  /**
   * note if we have not tokens to compute 3-gram probabilities we compute 2-gram or 1-gram probabilities.
   * the following code might be repetitive but more efficient than having the main loop outside.
   */

  //we have two valid past tokens available
  if(wd1It!=vocabCode.end() && wd2It!=vocabCode.end())
  {
    //iterate over all vocab words
    for(std::map<int,std::string>::const_iterator it = vocabDecode.begin(); it!=vocabDecode.end(); it++)
    {
      //if wd3 matches prefix and filter -> compute its backoff probability and add to the result set
      if(matchesPrefixAndFilter(it->second,prefix,filter))
      {
        std::pair<float,std::string> p;
        p.first = computeTrigramBackoff(wd1It->second,wd2It->second,it->first);
        p.second = it->second;
        result.insert(p);
      }
    }
  }

  //we have one valid past token available
  else if(wd2It!=vocabCode.end())
  {
    //iterate over all vocab words
    for(std::map<int,std::string>::const_iterator it = vocabDecode.begin(); it!=vocabDecode.end(); it++)
    {
      //if wd3 matches prefix and filter -> compute its backoff probability and add to the result set
      if(matchesPrefixAndFilter(it->second,prefix,filter))
      {
        std::pair<float,std::string> p;
        p.first = computeBigramBackoff(wd2It->second,it->first);
        p.second = it->second;
        result.insert(p);
      }
    }
  }

  //we have no valid past token available
  else
  {
    //iterate over all vocab words
    for(std::map<int,std::string>::const_iterator it = vocabDecode.begin(); it!=vocabDecode.end(); it++)
    {
      //if wd3 matches prefix and filter -> compute its backoff probability and add to the result set
      if(matchesPrefixAndFilter(it->second,prefix,filter))
      {
        std::pair<float,std::string> p;
        p.first = unigramMap.find(it->first)->second.logProb;
        p.second = it->second;
        result.insert(p);
      }
    }
  }


  size_t numSuggestions = 0;
  for(std::multimap<float,std::string>::const_iterator it = result.begin(); it != result.end() && numSuggestions < max_partial_prediction_size; ++it)
  {
    prediction.addSuggestion(Suggestion(it->second,exp(it->first)));
    numSuggestions++;
  }

  return prediction;
}
/**
 * Computes P(wd3 | wd1 wd2)
 */
float ARPAPlugin::computeTrigramBackoff(int wd1,int wd2,int wd3) const
{
  logger << DEBUG << "computing P( ["<<vocabDecode.find(wd3)->second<< "] | ["<<vocabDecode.find(wd1)->second<<"] ["<<vocabDecode.find(wd2)->second<<"] )"<<endl;

  //trigram exist
  std::map<TrigramKey,float>::const_iterator trigramIt =trigramMap.find(TrigramKey(wd1,wd2,wd3));
  if(trigramIt!=trigramMap.end())
  {
    logger << DEBUG << "trigram ["<<vocabDecode.find(wd1)->second<< "] ["<<vocabDecode.find(wd2)->second<< "] ["<<vocabDecode.find(wd3)->second<< "] exists" <<endl;
    logger << DEBUG << "returning "<<trigramIt->second <<endl;
    return trigramIt->second;
  }

  //bigram exist
  std::map<BigramKey,ARPAData>::const_iterator bigramIt =bigramMap.find(BigramKey(wd1,wd2));
  if(bigramIt!=bigramMap.end())
  {
    logger << DEBUG << "bigram ["<<vocabDecode.find(wd1)->second<< "] ["<<vocabDecode.find(wd2)->second<< "] exists" <<endl;
    float prob = bigramIt->second.logAlfa + computeBigramBackoff(wd2,wd3);
    logger << DEBUG << "returning "<<prob<<endl;
    return prob;
  }

  //else
  logger << DEBUG << "no bigram w1,w2 exist" <<endl;
  float prob = computeBigramBackoff(wd2,wd3);
  logger << DEBUG << "returning "<<prob<<endl;
  return prob;

}

/**
 * Computes P( wd2 | wd1 )
 */
float ARPAPlugin::computeBigramBackoff(int wd1, int wd2) const
{
  //bigram exist
  std::map<BigramKey,ARPAData>::const_iterator bigramIt =bigramMap.find(BigramKey(wd1,wd2));
  if(bigramIt!=bigramMap.end())
    return bigramIt->second.logProb;

  //else
  return unigramMap.find(wd1)->second.logAlfa +unigramMap.find(wd2)->second.logProb;

}




void ARPAPlugin::learn()
{
  logger << DEBUG << "learn() method called" << endl;
  logger << DEBUG << "learn() method exited" << endl;
}

void ARPAPlugin::extract()
{
    logger << DEBUG << "extract() method called" << endl;
    logger << DEBUG << "extract() method exited" << endl;
}

void ARPAPlugin::train()
{
    logger << DEBUG << "train() method called" << endl;
    logger << DEBUG << "train() method exited" << endl;
}
