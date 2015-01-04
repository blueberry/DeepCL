// tests the time for a single batch to go forward and backward through the layers

#include <iostream>
#include <random>

#include "NeuralNet.h"

#include "gtest/gtest.h"

#include "test/gtest_supp.h"
#include "test/Sampler.h"
#include "Timer.h"

using namespace std;

TEST( testsinglebatch, main ) {
    const int batchSize = 128;
    const float learningRate = 0.1f;

    NeuralNet *net = NeuralNet::maker()->planes(1)->boardSize(28)->instance();
    net->convolutionalMaker()->numFilters(32)->filterSize(5)->relu()->biased()->insert();
    net->convolutionalMaker()->numFilters(32)->filterSize(5)->relu()->biased()->insert();
    net->convolutionalMaker()->numFilters(10)->filterSize(20)->tanh()->biased()->insert();
    net->setBatchSize(batchSize);

    mt19937 random;
    random.seed(0); // so always gives same results
    const int inputsSize = net->getInputSizePerExample() * batchSize;
    float *inputData = new float[ inputsSize ];
    for( int i = 0; i < inputsSize; i++ ) {
        inputData[i] = random() / (float)mt19937::max() * 0.2f - 0.1f;
    }
    const int resultsSize = net->getLastLayer()->getResultsSize();
    float *expectedResults = new float[resultsSize];
    for( int i = 0; i < resultsSize; i++ ) {
        expectedResults[i] = random() / (float)mt19937::max() * 0.2f - 0.1f;
    }

    for (int layerIndex = 1; layerIndex <= 3; layerIndex++ ) {
        ConvolutionalLayer *layer = dynamic_cast<ConvolutionalLayer*>( net->layers[layerIndex] );
        int weightsSize = layer->getWeightsSize();
        for( int i = 0; i < weightsSize; i++ ) {
            layer->weights[i] = random() / (float)mt19937::max() * 0.2f - 0.1f;
        }
        layer->weightsWrapper->copyToDevice();
        int biasWeightsSize = layer->getBiasWeightsSize();
        for( int i = 0; i < biasWeightsSize; i++ ) {
            layer->biasWeights[i] = random() / (float)mt19937::max() * 0.2f - 0.1f;
        }
//        layer->biasWeightsWrapper->copyToDevice();
    }

    net->propagate( inputData );
    for (int layerIndex = 1; layerIndex <= 3; layerIndex++ ) {
        ConvolutionalLayer *layer = dynamic_cast<ConvolutionalLayer*>( net->layers[layerIndex] );
        float const*results = layer->getResults();
        cout << "layer " << layerIndex << endl;
        Sampler::printSamples( "results", resultsSize, (float*)results, 3 );        
    }
float *results = (float*)(net->layers[1]->getResults());
EXPECT_FLOAT_NEAR( 0.0767364, results[684] );
EXPECT_FLOAT_NEAR( 0.0294366, results[559] );
EXPECT_FLOAT_NEAR( 0.0244578, results[373] );
results = (float*)(net->layers[2]->getResults());
EXPECT_FLOAT_NEAR( 0.0867873, results[684] );
EXPECT_FLOAT_NEAR( 0.0496127, results[559] );
EXPECT_FLOAT_NEAR( 0, results[373] );
results = (float*)(net->layers[3]->getResults());
EXPECT_FLOAT_NEAR( -0.232493, results[684] );
EXPECT_FLOAT_NEAR( 0.179215, results[559] );
EXPECT_FLOAT_NEAR( 0.14498, results[373] );

float *weights = net->layers[3]->weights;
int weightsSize = net->layers[3]->getWeightsSize();
Sampler::printSamples( "weights", weightsSize, (float*)weights, 3 );        

EXPECT_FLOAT_NEAR( -0.0688307, weights[16044] );
EXPECT_FLOAT_NEAR( 0.0310387, weights[72239] );
EXPECT_FLOAT_NEAR( -0.0746839, weights[98933] );

cout << "=============== backprop ..." << endl;
float *errors = new float[100000];
ConvolutionalLayer *layer3 = dynamic_cast<ConvolutionalLayer*>( net->layers[3] );
layer3->calcErrors( expectedResults, errors );
int layer3ResultsSize = layer3->getResultsSize();
Sampler::printSamples( "errors", layer3ResultsSize, errors, 3 );        

EXPECT_FLOAT_NEAR( -0.296495, errors[684] );
EXPECT_FLOAT_NEAR( 0.214934, errors[559] );
EXPECT_FLOAT_NEAR( 0.1246, errors[373] );

cout << endl;

ConvolutionalLayer *layer2 = dynamic_cast<ConvolutionalLayer*>( net->layers[2] );
int layer2ResultsSize = layer2->getResultsSize();
float *layer2errors = new float[ layer2ResultsSize ];
layer3->backPropErrors( learningRate, errors, layer2errors );
Sampler::printSamples( "layer2errors", layer2ResultsSize, layer2errors );

EXPECT_FLOAT_NEAR( -0.296495, errors[684] );
EXPECT_FLOAT_NEAR( 0.214934, errors[559] );
EXPECT_FLOAT_NEAR( 0.1246, errors[373] );

EXPECT_FLOAT_NEAR( 0.00070503, layer2errors[1116844] );
EXPECT_FLOAT_NEAR( 0.0220331, layer2errors[174639] );
EXPECT_FLOAT_NEAR( -0.00368077, layer2errors[1353333] );
EXPECT_FLOAT_NEAR( -0.0292891, layer2errors[314560] );
EXPECT_FLOAT_NEAR( 0.0361823, layer2errors[176963] );

cout << endl;

ConvolutionalLayer *layer1 = dynamic_cast<ConvolutionalLayer*>( net->layers[1] );
int layer1ResultsSize = layer1->getResultsSize();
float *layer1errors = new float[ layer1ResultsSize ];
layer2->backPropErrors( learningRate, layer2errors, layer1errors );
Sampler::printSamples( "layer1errors", layer1ResultsSize, layer1errors );

EXPECT_FLOAT_NEAR( -0.0137842, layer1errors[199340] );
EXPECT_FLOAT_NEAR( -0.015897, layer1errors[567855] );
EXPECT_FLOAT_NEAR( 0.0170709, layer1errors[2270837] );

cout << endl;

layer1->backPropErrors( learningRate, layer1errors, 0 );

    net->backProp( learningRate, expectedResults );
    for (int layerIndex = 3; layerIndex >= 1; layerIndex-- ) {
        ConvolutionalLayer *layer = dynamic_cast<ConvolutionalLayer*>( net->layers[layerIndex] );
        weights = layer->weights;
        weightsSize = layer->getWeightsSize();
        cout << "layer " << layerIndex << endl;
        Sampler::printSamples( "weights", weightsSize, (float*)weights, 3 );        
        float *biasWeights = layer->biasWeights;
        int biasWeightsSize = layer->getBiasWeightsSize();
        cout << "layer " << layerIndex << endl;
        Sampler::printSamples( "biasWeights", biasWeightsSize, (float*)biasWeights, 3 );        
    }

weights = net->layers[3]->weights;
EXPECT_FLOAT_NEAR( -0.0679266, weights[16044] );
EXPECT_FLOAT_NEAR( 0.0284175, weights[72239] );
EXPECT_FLOAT_NEAR( -0.0749269, weights[98933] );
float *biasWeights = net->layers[3]->biasWeights;
EXPECT_FLOAT_NEAR( -0.0605856, biasWeights[4] );
EXPECT_FLOAT_NEAR( -0.0663593, biasWeights[9] );
EXPECT_FLOAT_NEAR( -0.0573801, biasWeights[3] );
weights = net->layers[2]->weights;
EXPECT_FLOAT_NEAR( 0.0507008, weights[16044] );
EXPECT_FLOAT_NEAR( 0.0982873, weights[21039] );
EXPECT_FLOAT_NEAR( -0.094224, weights[22133] );
biasWeights = net->layers[2]->biasWeights;
EXPECT_FLOAT_NEAR( -0.0552651, biasWeights[12] );
EXPECT_FLOAT_NEAR( -0.0571462, biasWeights[15] );
EXPECT_FLOAT_NEAR( -0.0304532, biasWeights[21] );
weights = net->layers[1]->weights;
EXPECT_FLOAT_NEAR( -0.0223737, weights[44] );
EXPECT_FLOAT_NEAR( -0.0658144, weights[239] );
EXPECT_FLOAT_NEAR( -0.0419252, weights[533] );
biasWeights = net->layers[1]->biasWeights;
EXPECT_FLOAT_NEAR( -0.0563513, biasWeights[12] );
EXPECT_FLOAT_NEAR( -0.0601025, biasWeights[15] );
EXPECT_FLOAT_NEAR( 0.000941529, biasWeights[21] );

    Timer timer;
    for( int i = 0; i < 3; i++ ) {
        net->learnBatch( learningRate, inputData, expectedResults );
    }
    timer.timeCheck("batch time");
    StatefulTimer::dump(true);

    results = (float*)(net->getResults());
    Sampler::printSamples( "net->getResults()", resultsSize, (float*)results );

EXPECT_FLOAT_NEAR( -0.15081, net->getResults()[684] );
EXPECT_FLOAT_NEAR( -0.0236106, net->getResults()[559] );
EXPECT_FLOAT_NEAR( -0.0585419, net->getResults()[373] );
EXPECT_FLOAT_NEAR( 0.168737, net->getResults()[960] );
EXPECT_FLOAT_NEAR( -0.00845184, net->getResults()[323] );

    delete[] expectedResults;
    delete[] inputData;
    delete net;
}
