#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <set>
#include <limits.h>
#include <random>
#include <chrono>
#include <iomanip> 
#include <cmath>
#include "BoostGraph.hpp"

using namespace std;
using namespace std::chrono;

const int SCORE_RESET_THRESHOLD = 1;
const int TIME_LIMIT = 500;  

bool connectedComponents = true;
bool twinsElimination = false;

struct PairHash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

struct ContractionStep {
    int iteration;
    pair<int, int> vertexPair;
    int score;
    int width;
};

struct ComponentSolution {
    ostringstream stringSequence;
    vector<ContractionStep> contractionSteps;
    int width;

    ComponentSolution(const ComponentSolution& other) {
        width = other.width;
        stringSequence.str(other.stringSequence.str());
        contractionSteps = other.contractionSteps;
    }

    ComponentSolution() = default;

    ComponentSolution& operator=(const ComponentSolution& other) {
        if (this == &other) return *this; 

        width = other.width;
        stringSequence.str("");
        stringSequence << other.stringSequence.str();

        contractionSteps = other.contractionSteps;

        return *this;
    }
};


class Graph {
private:
    vector<int> vertices;
    vector<int> ids; // mapping id -> index, used for connected components
    vector<vector<int>> adjListBlack;  // For black edges
    vector<vector<int>> adjListRed;    // For red edges
    vector<vector<int>> redDegreeToVertices; // vertex id saved
    vector<vector<int>> degreeToVertices;
    int width = 0;
    std::mt19937 gen;
    bool useFixedSeed = true;

public:
    Graph() {
        if(useFixedSeed) {
            gen.seed(12345);
        } else {
            std::random_device rd;
            gen.seed(rd());
        }
    }

    Graph(const Graph &g) : gen(12345) {
        this->vertices = g.vertices;
        this->ids = g.ids;
        this->adjListBlack = g.adjListBlack;
        this->adjListRed = g.adjListRed;
        this->redDegreeToVertices = g.redDegreeToVertices;
        this->degreeToVertices = g.degreeToVertices;
        this->width = g.width;

        if(useFixedSeed) {
            gen.seed(12345);
        } else {
            std::random_device rd;
            gen.seed(rd());
        }
    }

    int getRealScoreSimulate();

    void updateDegrees(int v){
        updateVertexRedDegree(v, 0);
        updateVertexDegree(v, 0);
    }

    int getVertexId(int v){
        return ids[v];
    }

    void addVertex(int v){
        vertices.push_back(v);
        updateVertexRedDegree(v, 0);
    }
        
    // Adds n vertices to the graph numbered from 0 to n-1
    void addVertices(int n){
        vertices.resize(n);
        adjListBlack.resize(n);
        adjListRed.resize(n);

        std::iota(vertices.begin(), vertices.end(), 0); // populate vertices with 0...n-1
        redDegreeToVertices.insert(redDegreeToVertices.begin(), vertices);
        // degreeToVertices.insert(degreeToVertices.begin(), vertices);
    }

    void addVertices(int n, vector<int> ids){
        adjListBlack.resize(n);
        adjListRed.resize(n);
        vertices.resize(n);

        this->ids = ids;
        std::iota(vertices.begin(), vertices.end(), 0); // populate vertices with 0...n-1
        redDegreeToVertices.insert(redDegreeToVertices.begin(), vertices);
        // degreeToVertices.insert(degreeToVertices.begin(), vertices);
    }

    void setIds(vector<int> values) {
        ids = values;
    }

    void addEdgeBegin(int v1, int v2) {
        if (v1 < v2) {
            adjListBlack[v2].push_back(v1);
            adjListBlack[v1].push_back(v2);
        }
    }

    void updateBlackDegrees() {
        for (int i = 0; i < adjListBlack.size(); ++i) {
            if (degreeToVertices.size() <= adjListBlack[i].size()) degreeToVertices.resize(adjListBlack[i].size() + 1);
            degreeToVertices[adjListBlack[i].size()].push_back(i);
        }
    }

    void addEdge(int v1, int v2, const string& color = "black") {
        if (color == "black" && std::find(adjListBlack[v1].begin(), adjListBlack[v1].end(), v2) == adjListBlack[v1].end()) {
            updateVertexDegree(v1, 1);
            updateVertexDegree(v2, 1);
            adjListBlack[v1].push_back(v2);
            adjListBlack[v2].push_back(v1);
        } else if (color == "red" && std::find(adjListRed[v1].begin(), adjListRed[v1].end(), v2) == adjListRed[v1].end()) {
            updateVertexDegree(v1, 1);
            updateVertexDegree(v2, 1);
            updateVertexRedDegree(v1, 1);
            updateVertexRedDegree(v2, 1);
            adjListRed[v1].push_back(v2);
            adjListRed[v2].push_back(v1);
        }
    }

    void removeEdge(int v1, int v2) {
        if (std::find(adjListBlack[v1].begin(), adjListBlack[v1].end(), v2) != adjListBlack[v1].end()) {
            // order matters since updateVertexDegree uses adjListBlack's state
            updateVertexDegree(v1, -1);
            updateVertexDegree(v2, -1);
            adjListBlack[v1].erase(std::remove(adjListBlack[v1].begin(), adjListBlack[v1].end(), v2), adjListBlack[v1].end());
            adjListBlack[v2].erase(std::remove(adjListBlack[v2].begin(), adjListBlack[v2].end(), v1), adjListBlack[v2].end());
        } else if (std::find(adjListRed[v1].begin(), adjListRed[v1].end(), v2) != adjListRed[v1].end()) {
            updateVertexDegree(v1, -1);
            updateVertexDegree(v2, -1);
            updateVertexRedDegree(v1, -1);
            updateVertexRedDegree(v2, -1);
            adjListRed[v1].erase(std::remove(adjListRed[v1].begin(), adjListRed[v1].end(), v2), adjListRed[v1].end());
            adjListRed[v2].erase(std::remove(adjListRed[v2].begin(), adjListRed[v2].end(), v1), adjListRed[v2].end());
        }
    }

    void removeVertex(int vertex) {        
        // Remove the vertex from the black adjacency list and update neighbors
        if (!adjListBlack[vertex].empty()) {
            vector<int> neighbors = adjListBlack[vertex];
            for (int neighbor : neighbors) {
                removeEdge(neighbor, vertex);
            }
        }
        
        // Remove the vertex from the red adjacency list and update neighbors
        if (!adjListRed[vertex].empty()) {
            vector<int> neighbors = adjListRed[vertex];
            for (int neighbor : neighbors) {
                removeEdge(neighbor, vertex);
            }
        }
        
        vertices.erase(std::find(vertices.begin(), vertices.end(), vertex));
        redDegreeToVertices[adjListRed[vertex].size()].erase(std::remove(redDegreeToVertices[adjListRed[vertex].size()].begin(), redDegreeToVertices[adjListRed[vertex].size()].end(), vertex));
        degreeToVertices[adjListBlack[vertex].size() + adjListRed[vertex].size()].erase(std::remove(degreeToVertices[adjListRed[vertex].size() + adjListBlack[vertex].size()].begin(), degreeToVertices[adjListRed[vertex].size() + adjListBlack[vertex].size()].end(), vertex));
    }

    int getWidth() const {
        return width;
    }

    vector<int> getVertices() {
        return vertices;
    }

    vector<int> getIds() {
        return this->ids;
    }

    bool isBipartiteBoost(std::vector<int>& partition1, std::vector<int>& partition2) {
        BoostGraph boostGraph(vertices.size());
        for (int i = 0; i < adjListBlack.size(); ++i) {
            for (int j = 0; j < adjListBlack[i].size(); ++j) {
                if (i < adjListBlack[i][j])  boostGraph.addEdge(i, adjListBlack[i][j]);
            }
        }

        return boostGraph.isBipartite(partition1, partition2);
    }

    std::vector<Graph> findConnectedComponentsBoost() {
        BoostGraph boostGraph(vertices.size());
        for (int i = 0; i < adjListBlack.size(); ++i) {
            for (int j = 0; j < adjListBlack[i].size(); ++j) {
                boostGraph.addEdge(i, adjListBlack[i][j]);
            }
        }

        vector<set<pair<int, int>>> components;
        vector<vector<int>> vertices;
        std::tie(components, vertices) = boostGraph.getConnectedComponentAndVertices();  // Call getConnectedComponents and unpack the result
        std::vector<Graph> result;
        if (components.size() == 1) {
            result.push_back(*this);
            return result;
        }

        for (int i = 0; i < components.size(); ++i) {
            Graph g;
            g.addVertices(vertices[i].size(), vertices[i]);
            constructFromEdges(g, components[i]);
            g.updateBlackDegrees();
            result.push_back(g);
        }
        for (const auto& edges : components) {
        }
        return result;
    }

    static void constructFromEdges(Graph& g, const std::set<std::pair<int, int>>& edges) {
        for (const auto& edge : edges) {
            int u = edge.first, v = edge.second;
            vector<int> ids = g.getIds();
            int vId = std::distance(ids.begin(), std::find(ids.begin(), ids.end(), v)); 
            int uId = std::distance(ids.begin(), std::find(ids.begin(), ids.end(), u)); 
            g.addEdgeBegin(vId, uId);
        }
    }

    float getDegreeDeviation() {
        int totalVertices = vertices.size();
        int totalDegree = 0;
        for(int i = 0; i < degreeToVertices.size(); ++i) {
            totalDegree += i * degreeToVertices[i].size();
        }
        float meanDegree = static_cast<float>(totalDegree) / totalVertices;

        float sumAbsoluteDeviations = 0.0;
        for(int i = 0; i < degreeToVertices.size(); ++i) {
            sumAbsoluteDeviations += abs(i - meanDegree) * degreeToVertices[i].size();
        }
        
        float averageDegreeDeviation = sumAbsoluteDeviations / totalVertices;
        return averageDegreeDeviation;
    }

    void updateVertexRedDegree(int vertex, int diff) {
        int oldDegree = adjListRed[vertex].size();
        int newDegree = oldDegree + diff;
        redDegreeToVertices[oldDegree].erase(std::remove(redDegreeToVertices[oldDegree].begin(), redDegreeToVertices[oldDegree].end(), vertex), redDegreeToVertices[oldDegree].end());
        
        if (redDegreeToVertices.size() <= newDegree) redDegreeToVertices.resize(newDegree + 1);
        redDegreeToVertices[oldDegree + diff].push_back(vertex);
    }

    void updateVertexDegree(int vertex, int diff) {
        int oldDegree = adjListRed[vertex].size() + adjListBlack[vertex].size();
        int newDegree = oldDegree + diff;
        degreeToVertices[oldDegree].erase(std::remove(degreeToVertices[oldDegree].begin(), degreeToVertices[oldDegree].end(), vertex), degreeToVertices[oldDegree].end());
        
        if (degreeToVertices.size() <= newDegree) degreeToVertices.resize(newDegree + 1);
        degreeToVertices[oldDegree + diff].push_back(vertex);
    }

    std::vector<int> getTopNVerticesWithLowestRedDegree(int n) {
        std::vector<int> topVertices;
        
        for (const auto& degreeVector : redDegreeToVertices) {
            for (int vertex : degreeVector) {
                if (topVertices.size() >= n) break;
                topVertices.push_back(vertex);
            }
            if (topVertices.size() >= n) break;
        }
        return topVertices;
    }

    std::vector<int> getTopNVerticesWithLowestDegree(int n) {
        std::vector<int> topVertices;
        
        for (const auto& degreeVector : degreeToVertices) {
            for (int vertex : degreeVector) {
                if (topVertices.size() >= n) break;
                topVertices.push_back(vertex);
            }
            if (topVertices.size() >= n) break;
        }
        return topVertices;
    }

    void mergeVertices(int source, int twin){
        auto start = high_resolution_clock::now();
        removeEdge(source, twin);
        transferRedEdges(twin, source);
        markUniqueEdgesRed(source, twin);
        addNewRedNeighbors(source, twin);
        removeVertex(twin);
        updateWidth();
    }

    void addNewRedNeighbors(int source, int twin) {
        // Merge red and black edges for both source and twin
        vector<int> mergedSourceNeighbors(adjListBlack[source].begin(), adjListBlack[source].end());
        std::vector<int> mergedTwinNeighbors(adjListBlack[twin].begin(), adjListBlack[twin].end());

        sort(mergedSourceNeighbors.begin(), mergedSourceNeighbors.end());
        sort(mergedTwinNeighbors.begin(), mergedTwinNeighbors.end());


        // Find edges of twin that are not adjacent to source
        std::vector<int> newRedEdges;
        std::set_difference(
            mergedTwinNeighbors.begin(), mergedTwinNeighbors.end(),
            mergedSourceNeighbors.begin(), mergedSourceNeighbors.end(),
            std::inserter(newRedEdges, newRedEdges.end())
        );

        // Add these edges as red edges for source
        for (int v : newRedEdges) {
            addEdge(source, v, "red");
        }
    }


    void transferRedEdges(int fromVertex, int toVertex) {
        // If the twin vertex has red edges
        if(!adjListRed[fromVertex].empty()) {
            for (int vertex : adjListRed[fromVertex]) {
                if (std::find(adjListRed[toVertex].begin(), adjListRed[toVertex].end(), vertex) == adjListRed[toVertex].end()) {
                    addEdge(toVertex, vertex, "red");
                }
            }
        }
    }

    void deleteTransferedEdges(int vertex, vector<int> neighbors) {
        if(!neighbors.empty()) {
            for (int neighbor : neighbors) {
                removeEdge(vertex, neighbor);
            }
        }
    }

    void markUniqueEdgesRed(int source, int twin) {
        vector<int> source_neighbors(adjListBlack[source].begin(), adjListBlack[source].end());
        vector<int> twin_neighbors(adjListBlack[twin].begin(), adjListBlack[twin].end());

        sort(source_neighbors.begin(), source_neighbors.end());
        sort(twin_neighbors.begin(), twin_neighbors.end());

        vector<int> toBecomeRed;
        std::set_difference(
            source_neighbors.begin(), source_neighbors.end(),
            twin_neighbors.begin(), twin_neighbors.end(),
            std::inserter(toBecomeRed, toBecomeRed.begin())
        );

        for (int v : toBecomeRed) {
            removeEdge(source, v);
            addEdge(source, v, "red");
        }
    }

    int getScore(int v1, int v2) {
        vector<int> neighbors_v1 = adjListBlack[v1];
        if (!adjListRed[v1].empty()) {
            neighbors_v1.insert(neighbors_v1.end(), adjListRed[v1].begin(), adjListRed[v1].end());
        }

        vector<int> neighbors_v2 = adjListBlack[v2];
        if (!adjListRed[v2].empty()) {
            neighbors_v2.insert(neighbors_v2.end(), adjListRed[v2].begin(), adjListRed[v2].end());
        }

        sort(neighbors_v1.begin(), neighbors_v1.end());
        sort(neighbors_v2.begin(), neighbors_v2.end());

        vector<int> symmetric_difference;
        std::set_symmetric_difference(neighbors_v1.begin(), neighbors_v1.end(),
                                    neighbors_v2.begin(), neighbors_v2.end(),
                                    std::inserter(symmetric_difference, symmetric_difference.begin()));
        auto it1 = std::find(symmetric_difference.begin(), symmetric_difference.end(), v1);
        if (it1 != symmetric_difference.end()) {
            symmetric_difference.erase(it1);
        }

        auto it2 = std::find(symmetric_difference.begin(), symmetric_difference.end(), v2);
        if (it2 != symmetric_difference.end()) {
            symmetric_difference.erase(it2);
        }

        return symmetric_difference.size();
    }

    int getRandomDistance() {
        std::uniform_int_distribution<> distrib(1, 2);
        return distrib(gen);
    }

    int getRandomNeighbor(int vertex) {
        vector<int> allNeighbors;
        allNeighbors.insert(allNeighbors.end(), adjListBlack[vertex].begin(), adjListBlack[vertex].end());
        allNeighbors.insert(allNeighbors.end(), adjListRed[vertex].begin(), adjListRed[vertex].end());

        std::shuffle(allNeighbors.begin(), allNeighbors.end(), gen);
        return allNeighbors[0];
    }

    set<int> getRandomWalkVertices(int vertex, int numberVertices) {
        set<int> randomWalkVertices;
        for (int i = 0; i < numberVertices; ++i) {
            int distance = getRandomDistance();            
            int randomVertex = getRandomNeighbor(vertex);
            if (distance == 2 && adjListBlack[randomVertex].size() + adjListRed[randomVertex].size() != 0) randomVertex = getRandomNeighbor(randomVertex);
            randomWalkVertices.insert(randomVertex);
        }
        randomWalkVertices.erase(vertex);
        return randomWalkVertices;
    }

    set<int> getRandomStep(int vertex, int numberVertices) {
        set<int> randomWalkVertices;
        for (int i = 0; i < numberVertices; ++i) {
            int randomVertex = getRandomNeighbor(vertex);
            randomWalkVertices.insert(randomVertex);
        }
        randomWalkVertices.erase(vertex);
        return randomWalkVertices;
    }

    ostringstream findTwins(bool trueTwins) {
        ostringstream contractionSequence;        
        
        vector<vector<int>> true_partitions; 
        vector<vector<int>> updated_true_partitions; 
        true_partitions.push_back(vertices);

        vector<vector<int>> false_partitions; 
        vector<vector<int>> updated_false_partitions; 
        false_partitions.push_back(vertices);

        for (int v : vertices) {
            vector<int> neighbors = adjListBlack[v];
            sort(neighbors.begin(), neighbors.end());
            for (vector partition : true_partitions) {
                vector<int> difference;
                std::set_difference(partition.begin(), partition.end(), neighbors.begin(), neighbors.end(), 
                        std::inserter(difference, difference.end()));

                vector<int> intersection;
                std::set_intersection(partition.begin(), partition.end(), neighbors.begin(), neighbors.end(), 
                        std::inserter(intersection, intersection.end()));

                if (!difference.empty()) updated_true_partitions.push_back(difference);
                if (!intersection.empty()) updated_true_partitions.push_back(intersection);
            }
            true_partitions = updated_true_partitions;
            updated_true_partitions.clear();

            neighbors = adjListBlack[v];
            neighbors.push_back(v);
            sort(neighbors.begin(), neighbors.end());
            for (vector partition : false_partitions) {
                vector<int> difference;
                std::set_difference(partition.begin(), partition.end(), neighbors.begin(), neighbors.end(), 
                        std::inserter(difference, difference.end()));

                vector<int> intersection;
                std::set_intersection(partition.begin(), partition.end(), neighbors.begin(), neighbors.end(), 
                        std::inserter(intersection, intersection.end()));

                if (!difference.empty()) updated_false_partitions.push_back(difference);
                if (!intersection.empty()) updated_false_partitions.push_back(intersection);
            }
            false_partitions = updated_false_partitions;
            updated_false_partitions.clear();

        }

        for (const vector<int>& partition : true_partitions) {
            if (partition.size() > 1) {
                auto it = partition.begin();
                int first = *it;
                ++it;
                while(it != partition.end()) {
                    int next = *it;
                    contractionSequence << getVertexId(first) + 1 << " " << getVertexId(next) + 1 << "\n";
                    mergeVertices(first, next); 
                    cout << "c Found true twins" << endl;
                    ++it;
                }
            }
        }

        for (const vector<int>& partition : false_partitions) {
            if (partition.size() > 1) {
                auto it = partition.begin();
                int first = *it;
                ++it;
                while(it != partition.end()) {
                    int next = *it;
                    contractionSequence << getVertexId(first) + 1 << " " << getVertexId(next) + 1 << "\n";
                    mergeVertices(first, next); 
                    cout << "c Found false twins" << endl;
                    ++it;
                }
            }
        }
        return contractionSequence;
    }


    ostringstream findRedDegreeContractionRandomWalk(){ 
        ostringstream contractionSequence;
        vector<vector<pair<int, int>>> scores(*max_element(vertices.begin(), vertices.end()) + 1); // because of twin rule
        auto heuristic_start_time = high_resolution_clock::now();
        
        int iterationCounter = 0;
        while (vertices.size() > 1) {
            auto start = high_resolution_clock::now();

            vector<int> lowestDegreeVertices = getTopNVerticesWithLowestRedDegree(20);

            int bestScore = INT_MAX;
            pair<int, int> bestPair;

            for (int i = 0; i < lowestDegreeVertices.size(); i++) {
                int v1 = lowestDegreeVertices[i];
                set<int> randomWalkVertices = getRandomWalkVertices(v1, 10);  
              
                for (int v2 : randomWalkVertices) {
                    if (v2 > v1) {
                        std::swap(v1, v2);
                    }

                    auto it = find_if(scores[v1].begin(), scores[v1].end(),
                                    [v2](const pair<int, int>& p){ return p.first == v2; });
                    int score;
                    if (it != scores[v1].end()) {
                        score = it->second;
                    }
                    else {
                        score = getScore(v1, v2);
                        scores[v1].push_back({v2, score});
                    }
                    
                    if (score < bestScore) {
                        bestScore = score;
                        bestPair = {v1, v2};
                    }
                }
            }

            contractionSequence << getVertexId(bestPair.first) + 1 << " " << getVertexId(bestPair.second) + 1 << "\n";

            mergeVertices(bestPair.first, bestPair.second);

            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            int seconds_part = duration.count() / 1000;
            int milliseconds_part = duration.count() % 1000;
            std::cout << "c (Left " << vertices.size() << ", tww: " << getWidth() << ") Cycle in " << seconds_part << "." 
            << std::setfill('0') << std::setw(9) << milliseconds_part 
            << " seconds" << std::endl;

            if (++iterationCounter >= SCORE_RESET_THRESHOLD) {
                for(int i = 0; i < scores.size(); ++i) {
                    scores[i].clear();
                }
                iterationCounter = 0;
                cout << "c Reset at " << vertices.size() << endl;
            }
        }
        return contractionSequence;
    }

    ostringstream findDegreeContraction(){ 
        ostringstream contractionSequence;
        vector<vector<pair<int, int>>> scores(*max_element(vertices.begin(), vertices.end()) + 1); // because of twin rule
        auto heuristic_start_time = high_resolution_clock::now();
        
        int iterationCounter = 0;
        while (vertices.size() > 1) {
            auto start = high_resolution_clock::now();

            vector<int> lowestDegreeVertices = getTopNVerticesWithLowestDegree(20);
            
            int bestScore = INT_MAX;
            pair<int, int> bestPair;

            for (int i = 0; i < lowestDegreeVertices.size(); i++) {
                for (int j = i+1; j < lowestDegreeVertices.size(); j++) {
                    int v1 = lowestDegreeVertices[i];
                    int v2 = lowestDegreeVertices[j];

                    if (v2 > v1) {
                        std::swap(v1, v2);
                    }

                    auto it = find_if(scores[v1].begin(), scores[v1].end(),
                                    [v2](const pair<int, int>& p){ return p.first == v2; });
                    int score;
                    if (it != scores[v1].end()) {
                        score = it->second;
                    }
                    else {
                        score = getScore(v1, v2);
                        scores[v1].push_back({v2, score});
                    }
                    
                    if (score < bestScore) {
                        bestScore = score;
                        bestPair = {v1, v2};
                    }
                }
            }

            contractionSequence << getVertexId(bestPair.first) + 1 << " " << getVertexId(bestPair.second) + 1 << "\n";
            mergeVertices(bestPair.first, bestPair.second);


            if (++iterationCounter >= SCORE_RESET_THRESHOLD) {
                for(int i = 0; i < scores.size(); ++i) {
                    scores[i].clear();
                }
                iterationCounter = 0;
                cout << "c Reset at " << vertices.size() << endl;
            }

            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            int seconds_part = duration.count() / 1000;
            int milliseconds_part = duration.count() % 1000;
            std::cout << "c (Merged ( " << getVertexId(bestPair.first) << "," << getVertexId(bestPair.second) << "), left " << vertices.size() << ", tww: " << getWidth() << ") Cycle in " << seconds_part << "." 
            << std::setfill('0') << std::setw(9) << milliseconds_part 
            << " seconds" << std::endl;
        }
        return contractionSequence;
    }

private:
    void updateWidth() {
        for (int i = redDegreeToVertices.size() - 1; i >= 0; i--) {
            if (!redDegreeToVertices[i].empty()) {
                width = max(width ,i);
                break;
            }
        }
    }

    int getUpdatedWidth() {
        int updatedWidth = 0;
        for (const auto& innerVector : adjListRed) {
            updatedWidth = max(updatedWidth, static_cast<int>(innerVector.size()));
        }
        return updatedWidth;
    }
};

string getLastLine(ostringstream& oss) {
    const string& s = oss.str();
    auto lastNewlinePos = s.rfind('\n', s.length() - 2); // Start search before the very last character, which is likely a newline.
    if (lastNewlinePos != string::npos) {
        return s.substr(lastNewlinePos + 1); // Returns string after the last newline.
    } else {
        return s; // Return the entire string if there's no newline (i.e., it's a one-line string).
    }
}

int main() {
    Graph g;
    BoostGraph boostGraph;
    string line;
    int numVertices, numEdges;
    set<pair<int, int>> readEdges;
    double density;
    int maxTww = 0;
    bool constructComplement = false;

    auto start = high_resolution_clock::now(); 

    while (getline(cin, line)) {
        if (line[0] == 'c') {
            continue;
        }

        vector<string> tokens;
        string token;
        std::stringstream tokenStream(line);
        while (tokenStream >> token) {
            tokens.push_back(token);
        }

        if (tokens[0] == "p") {
            numVertices = stoi(tokens[2]);
            numEdges = stoi(tokens[3]);
            g.addVertices(numVertices);

            density = (2.0 * numEdges) / (numVertices * (numVertices - 1));
            if (density > 0.5) {
                constructComplement = true;
            }
        } else if (constructComplement) {
            int u = stoi(tokens[0]);
            int v = stoi(tokens[1]);
            readEdges.insert({min(u-1, v-1), max(u-1, v-1)});
        } else {
            int u = stoi(tokens[0]);
            int v = stoi(tokens[1]);
            g.addEdgeBegin(u - 1, v - 1);
            boostGraph.addEdge(u - 1, v - 1);
        }
    }

    if (constructComplement) {
        for (int i = 0; i < numVertices; i++) {
            for (int j = i + 1; j < numVertices; j++) {
                if (readEdges.find({i, j}) == readEdges.end()) {
                    g.addEdgeBegin(i, j);
                    boostGraph.addEdge(i, j);
                }
            }
        }
    }
    g.updateBlackDegrees();
    g.setIds(g.getVertices());

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(stop - start);
    std::cout << "c Time taken too initialize the graph: " << duration.count() << " seconds" << std::endl;

    start = high_resolution_clock::now(); 
    
    vector<Graph> components;
    if (connectedComponents) {
        components = g.findConnectedComponentsBoost();
    }
    else {
        components.push_back(g);
    }
    
    stop = high_resolution_clock::now();
    duration = duration_cast<seconds>(stop - start);
    cout << "c Time taken for connected components: " << duration.count() << " seconds" << std::endl;

    std::vector<int> remainingVertices;
    for (Graph& c : components) {
        ostringstream componentContraction;
        vector<int> partition1;
        vector<int> partition2;

        if (twinsElimination) {
            auto twin_start = high_resolution_clock::now();
            ostringstream twins = c.findTwins(false);
            cout << twins.str();
            auto twin_stop = high_resolution_clock::now();
            auto twin_duration = duration_cast<seconds>(twin_stop - twin_start);
            cout << "c Time taken for twins detection: " << twin_duration.count() << " seconds" << std::endl;
        }

        float degreeDeviation = c.getDegreeDeviation();
        cout << "c Deviation: " << degreeDeviation << endl;

        if (degreeDeviation <= 25.0) cout << c.findRedDegreeContractionRandomWalk().str();
        else cout << c.findDegreeContraction().str();

        maxTww = max(maxTww, c.getWidth());

        if (c.getVertices().size() == 1){
            int remainingVertex = c.getVertexId(*c.getVertices().begin()) + 1;
            remainingVertices.push_back(remainingVertex);
        }
        else {
            // Extract here the last remaining vertex from the findRedDegreeContraction's output and push it back to remaining vertices
            string lastLine = getLastLine(componentContraction);
            stringstream lastPair(lastLine);
            int remainingVertex;
            lastPair >> remainingVertex;
            remainingVertices.push_back(remainingVertex);
        }
    }

    int primaryVertex = remainingVertices[0];
    for (size_t i = 1; i < remainingVertices.size(); ++i) {
        cout << primaryVertex << " " << remainingVertices[i] << endl;
    }

    auto final_stop = high_resolution_clock::now();
    auto final_duration = duration_cast<seconds>(final_stop - start);
    std::cout << "c In total: " << final_duration.count() << " seconds" << std::endl;
    cout << "c twin-width: " << maxTww << endl;
    return 0;    
}

