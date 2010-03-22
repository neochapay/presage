
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


#include "core/predictorActivator.h"
#include "core/utility.h"

const char* PredictorActivator::LOGGER = "Presage.PredictorActivator.LOGGER";
const char* PredictorActivator::PREDICT_TIME = "Presage.PredictorActivator.PREDICT_TIME";
const char* PredictorActivator::MAX_PARTIAL_PREDICTION_SIZE = "Presage.PredictorActivator.MAX_PARTIAL_PREDICTION_SIZE";
const char* PredictorActivator::COMBINATION_POLICY = "Presage.PredictorActivator.COMBINATION_POLICY";

PredictorActivator::PredictorActivator(Configuration* configuration,
		     PluginRegistry* registry,
		     ContextTracker* ct)
    : config(configuration),
      pluginRegistry(registry),
      contextTracker(ct),
      logger("PredictorActivator", std::cerr),
      dispatcher(this)
{
    combiner = 0;

    // build notification dispatch map
    dispatcher.map (config->find (LOGGER), & PredictorActivator::setLogger);
    dispatcher.map (config->find (PREDICT_TIME), & PredictorActivator::setPredictTime);
    dispatcher.map (config->find (COMBINATION_POLICY), & PredictorActivator::setCombinationPolicy);
    dispatcher.map (config->find (MAX_PARTIAL_PREDICTION_SIZE), & PredictorActivator::setMaxPartialPredictionSize);
}


PredictorActivator::~PredictorActivator()
{
    delete combiner;
}

Prediction PredictorActivator::predict(unsigned int multiplier, const char** filter)
{
    Prediction result;

    // Here goes code to instantiate a separate thread for each Plugin
    //

    // All threads need to be synched together. One thread makes sure that
    // we are not exceeding the maximum time allowed.
    //

    // Now that the all threads have exited or have been cancelled,
    // the predictions returned by each of them are combined.
    //

    // clear out previous predictions
    predictions.clear();

    PluginRegistry::Iterator it = pluginRegistry->iterator();
    Plugin* plugin = 0;
    while (it.hasNext()) {
	plugin = it.next();
	logger << DEBUG << "Invoking predictive plugin: " << plugin->getName() << endl;
	predictions.push_back(plugin->predict(max_partial_prediction_size * multiplier, filter));
    }

    // ...then merge predictions into a single one...
    result = combiner->combine(predictions);

    // ...and return final prediction
    return result;

    ////////
    // PLUMP
    //
    //plump.registerCallback(callback_predict, &p);
    //plump.run();
}


void PredictorActivator::setLogger (const std::string& value)
{
    logger << setlevel (value);
    logger << INFO << "LOGGER: " << value << endl;
}


void PredictorActivator::setPredictTime (const std::string& value)
{
    int result = toInt (value);
    // handle exception where predictTime is less than zero
    if (result < 0) {
        logger << ERROR << "Error: attempted to set PREDICT_TIME option to "
	       << "a negative integer value. Please make sure that "
	       << "PREDICT_TIME option is set to a value greater "
	       << "than or equal to zero.\a" << endl;
    } else {
	logger << INFO << "PREDICT_TIME: " << result << endl;
        predict_time = result;
    }
}


int PredictorActivator::getPredictTime() const
{
    return predict_time;
}


void PredictorActivator::setCombinationPolicy(const std::string& cp)
{
    logger << INFO << "Setting COMBINATION_POLICY to " << cp << endl;
    delete combiner;
    combinationPolicy = cp;

    std::string policy = strtolower (cp);
    if (policy == "meritocracy") {
	combiner = new MeritocracyCombiner();
    } else {
	// TODO: throw exception
	logger << ERROR << "Error - unknown combination policy: "
	       << cp << endl;
    }
}


std::string PredictorActivator::getCombinationPolicy() const
{
    return combinationPolicy;
}


void PredictorActivator::setMaxPartialPredictionSize (const std::string& size)
{
    max_partial_prediction_size = toInt(size);
    logger << INFO << "MAX_PARTIAL_PREDICTION_SIZE: " << max_partial_prediction_size << endl;
}


void PredictorActivator::update (const Observable* variable)
{
    Variable* var = (Variable*) variable;
    
    logger << DEBUG << "About to invoke dispatcher: " << var->string() << " - " << var->get_value() << endl;

    dispatcher.dispatch (var);
}
