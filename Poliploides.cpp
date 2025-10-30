#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <ctime>
#include <map>
#include <matplot/matplot.h>
#include <queue>
#include <set>

using namespace std;
using namespace matplot;

// ESTRUCTURAS DE DATOS DEL ESCENARIO

/*
 Operation
 Representa una operacion individual dentro de un trabajo (Job).

 id: Identificador unico de la operacion.
*/
struct Operation {
    int id;
    
    Operation() : id(-1) {}
    Operation(int _id) : id(_id) {}
};

/*
 Job
 Representa un trabajo que contiene una secuencia de operaciones a realizar.

 id: Identificador unico del trabajo.
 operations: Lista de identificadores de operaciones asociadas a este trabajo.
 
 
 addOperation(int opId): Agrega una nueva operacion al trabajo.
*/
struct Job {
    int id;
    vector<int> operations;
    
    Job() : id(-1) {}
    Job(int _id) : id(_id) {}
    
    void addOperation(int opId) {
        operations.push_back(opId);
    }
};

struct ScenarioData {
    int numOperations;
    int numMachines;
    int numJobs;
    
    vector<vector<double>> processingTime;
    vector<vector<double>> energyCost;
    vector<Job> jobs;
    map<string,vector<pair<Job,Operation>>> chromosomeMapping;
    
    ScenarioData() : numOperations(5), numMachines(4), numJobs(0) {}
};



/*
 MachineState
 Representa el estado actual de una maquina durante la simulacion
 
 currentTime: Tiempo en que la maquina estara libre para una nueva operacion
 totalEnergy: Energia total consumida por esta maquina
 isActive: Indica si la maquina tiene operaciones asignadas
 */
struct MachineState {
    double currentTime;
    double totalEnergy;
    bool isActive;
    
    MachineState() : currentTime(0.0), totalEnergy(0.0), isActive(false) {}
};
/*
 OperationSchedule
 Representa una operacion programada con toda su informacion de asignacion
 
 operationId: ID de la operacion (indice en matrices de tiempo/energia)
 jobId: ID del trabajo al que pertenece
 machineId: Maquina asignada a esta operacion
 startTime: Tiempo de inicio de la operacion
 endTime: Tiempo de finalizacion de la operacion
 processingTime: Duracion de procesamiento
 energyCost: Consumo energetico de esta operacion
 */
struct OperationSchedule {
    int operationId;
    int jobId;
    int machineId;
    double startTime;
    double endTime;
    double processingTime;
    double energyCost;
    
    OperationSchedule() 
        : operationId(-1), jobId(-1), machineId(-1),
          startTime(0.0), endTime(0.0), 
          processingTime(0.0), energyCost(0.0) {}
};

/*
 JobState
 Mantiene el estado de progreso de cada trabajo durante la simulacion
 
 nextOperationIndex: indice de la siguiente operacion del trabajo que debe ejecutarse
 lastOperationEndTime: Tiempo de finalizacion de la ultima operacion ejecutada
 */
struct JobState {
    int nextOperationIndex;
    double lastOperationEndTime;
    
    JobState() : nextOperationIndex(0), lastOperationEndTime(0.0) {}
};

// REPRESENTACIoN DEL INDIVIDUO POLIPLOIDE

/*
 Chromosome
 Representa un cromosoma individual que codifica una politica de despacho
 
 Cada cromosoma almacena una secuencia de prioridades (valores enteros)
 que determinan el orden de ejecucion de las operaciones segun la politica
 
 policyName: Nombre de la politica (FIFO, LTP, STP, etc.)
 genes: Vector de enteros que codifican las prioridades
        - Tamaño variable segun el numero de operaciones
        - Valores tipicamente en rango [1, numMachines]
 */

void printHeader(const string& headerText, int length){
    #if defined(_WIN32)
        char topLeft = 201;  // ╔
        char topRight = 187; // ╗
        char bottomLeft = 200; // ╚
        char bottomRight = 188; // ╝
        char horizontal = 205; // ═
        char vertical = 186; // ║
    #else
        string topLeft = "╔";
        string topRight = "╗";
        string bottomLeft = "╚";
        string bottomRight = "╝";
        string horizontal = "═";
        string vertical = "║";
    #endif
        int maxSizePerLine = length - (length*2/5);
        int space = 0;
        vector<string> lines;
        stringstream ss(headerText);
        string word;
        string current;
    
        while (ss >> word) {
            if (current.size() + word.size() + 1 > maxSizePerLine) {
                lines.push_back(current);
                current = word; 
            } else {
                if (!current.empty())
                    current += " ";
                current += word;
            }
        }
        if (!current.empty())
            lines.push_back(current);
    
        cout << topLeft;
        for (int i=0;i<length;i++){
            cout << horizontal;
        }
        cout << topRight << endl;
        
        
        for (const string& line : lines) {
            space = (length-line.size())/2;
            cout << vertical;
            for (int i=0;i<space;i++){
                cout << " ";
            }
            cout << line;
            if (line.size()%2 != 0)
                cout << " ";
            for (int i=0;i<space;i++){
                cout << " ";
            }
            cout << vertical << endl;
        }
    
        cout << bottomLeft;
        for (int i=0;i<length;i++){
            cout << horizontal;
        }
        cout << bottomRight << endl << endl;
}

void printSubHeader(const string& subHeaderText, int length){
    #if defined(_WIN32)
        char left = 204;  // ╠
        char right = 185; // ╣
        char horizontal = 205; // ═
    #else
        string left = "╠";
        string right = "╣";
        string horizontal = "═";
    #endif
        int space = (length - subHeaderText.size())/2;
        cout << left;
        for (int i=0;i<space;i++){
            cout << horizontal;
        }
        cout << subHeaderText;
        if (subHeaderText.size()%2 != 0)
            cout << horizontal;
        for (int i=0;i<space;i++){
            cout << horizontal;
        }
        cout << right << endl << endl;
}

void printDivider(int length){
    #if defined(_WIN32)
        char left = 204;  // ╠
        char right = 185; // ╣
        char horizontal = 205; // ═
    #else
        string left = "╠";
        string right = "╣";
        string horizontal = "═";
    #endif
        cout << endl << left;
        for (int i=0;i<length;i++){
            cout << horizontal;
        }
        cout << right << endl;
}

void printTable(const vector<string>& fields, const vector<vector<string>>& values){
    #if defined(_WIN32)
        char vertical = 179;
        char horizontal = 196;
        char topLeft = 218;
        char topRight = 191;
        char bottomLeft = 192;
        char bottomRight = 217;
        char middleTop = 194;
        char middleMiddle = 197;
        char middleBottom = 193;
        char middleLeft = 195;
        char middleRight = 180;
    #else
        string vertical = "│";
        string horizontal = "─";
        string topLeft = "┌";
        string topRight = "┐";
        string bottomLeft = "└";
        string bottomRight = "┘";
        string middleTop = "┬";
        string middleMiddle = "┼";
        string middleBottom = "┴";
        string middleLeft = "├";
        string middleRight = "┤";
    #endif
    int spaces = 0;
    vector<int> columnWidths;
    int numFields = fields.size();


    for (const string& field : fields) {
        columnWidths.push_back(field.size() + 6);
        for (const auto& row : values) {
            int cellSize = row[&field - &fields[0]].size() + 6;
            if (cellSize > columnWidths.back()) {
                columnWidths.back() = cellSize;
            }
        }
    }
    cout << endl << topLeft;
    for (int i = 0; i < numFields; i++) {
        for (int j = 0; j < columnWidths[i]; j++) {
            cout << horizontal;
        }
        if (i < columnWidths.size() - 1)
            cout << middleTop;
    }
    cout << topRight << endl;
    
    for (int i = 0; i < numFields; i++) {
        spaces = (columnWidths[i] - fields[i].size()) / 2;
        cout << vertical;
        for (int j = 0; j < spaces; j++) {
            cout << " ";
        }
        cout << fields[i];
        for (int j = 0; j < spaces; j++) {
            cout << " ";
        }
        if ((columnWidths[i] - fields[i].size()) % 2 != 0) {
            cout << " ";
        }
    }
    cout << vertical << endl << middleLeft;
    for (int i = 0; i < numFields; i++) {
        for (int j = 0; j < columnWidths[i]; j++) {
            cout << horizontal;
        }
        if (i < columnWidths.size() - 1)
            cout << middleMiddle;
    }
    cout << middleRight << endl;
    for (const auto& row : values) {
        for (int i = 0; i < columnWidths.size(); i++) {
            int spaces = (columnWidths[i] - row[i].size()) / 2;
            cout << vertical;
            for (int j = 0; j < spaces; j++) {
                cout << " ";
            }
            cout << row[i];
            for (int j = 0; j < spaces; j++) {
                cout << " ";
            }
            if ((columnWidths[i] - row[i].size()) % 2 != 0) {
                cout << " ";
            }
        }
        cout << vertical << endl;
    }
    cout << bottomLeft;
    for (int i = 0; i < numFields; i++) {
        for (int j = 0; j < columnWidths[i]; j++) {
            cout << horizontal;
        }
        if (i < columnWidths.size() - 1)
            cout << middleBottom;
    }
    cout << bottomRight << endl;
}

class Chromosome {
public:
    string policyName;
    vector<int> genes;
    double f1; // Makespan
    double f2; // Energia total
    int domLevel; // Nivel de dominancia
    double crowdingDistance; // Distancia de Crowding
    
    // Constructor por defecto
    Chromosome() : policyName("") {
        f1 = 0.0;
        f2 = 0.0;
        domLevel = -1;
        crowdingDistance = -1;
    }
    
    // Constructor con nombre de politica
    Chromosome(const string& name) : policyName(name) {
        f1 = 0.0;
        f2 = 0.0;
        domLevel = -1;
        crowdingDistance = -1;
    }
    
    /*
     Inicializa el cromosoma con valores aleatorios
     
     size: Numero de genes (operaciones totales en el escenario)
     minValue: Valor minimo para los genes (tipicamente 1)
     maxValue: Valor maximo para los genes (tipicamente numMachines)
     rng: Generador de numeros aleatorios
     */
    void initializeRandom(int size, int minValue, int maxValue, mt19937& rng) {
        genes.clear();
        genes.reserve(size);
        
        uniform_int_distribution<int> dist(minValue, maxValue);
        
        for (int i = 0; i < size; i++) {
            genes.push_back(dist(rng));
        }
    }
    
    /*
     Obtiene el tamaño del cromosoma
     int: Numero de genes
     */
    int size() const {
        return genes.size();
    }
    
    /*
     Imprime el cromosoma en formato legible
     */
    void print() const {
        cout << policyName << ": [";
        for (size_t i = 0; i < genes.size(); i++) {
            cout << genes[i];
            if (i < genes.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
};

/*
 Individual
 Representa un individuo poliploide completo en el algoritmo genetico
 
 Un individuo contiene exactamente 6 cromosomas, uno por cada politica:
 - Cromosoma 0: FIFO (First In First Out)
 - Cromosoma 1: LTP (Longest Processing Time)
 - Cromosoma 2: STP (Shortest Processing Time)
 - Cromosoma 3: RRFIFO (Round Robin FIFO)
 - Cromosoma 4: RRLTP (Round Robin LTP)
 - Cromosoma 5: RRECA (Round Robin Energy Cost Aware)
 
 chromosomes: Vector de 6 cromosomas (tamaño fijo)
 f1: Fitness objetivo 1 - Makespan (tiempo total de finalizacion)
 f2: Fitness objetivo 2 - Consumo energetico total
 */
class Individual {
public:
    vector<Chromosome> chromosomes;
    
    // Constructor por defecto
    Individual() {
        // Inicializar con 6 cromosomas vacios
        chromosomes.resize(6);
        chromosomes[0] = Chromosome("FIFO");
        chromosomes[1] = Chromosome("LTP");
        chromosomes[2] = Chromosome("STP");
        chromosomes[3] = Chromosome("RRFIFO");
        chromosomes[4] = Chromosome("RRLTP");
        chromosomes[5] = Chromosome("RRECA");
    }
    
    /*
     Inicializa el individuo con valores aleatorios para todos sus cromosomas
     
     chromosomeSize: Numero de genes por cromosoma (total de operaciones)
     minValue: Valor minimo para los genes (tipicamente 1)
     maxValue: Valor maximo para los genes (tipicamente numMachines)
     rng: Generador de numeros aleatorios
     */
    void initializeRandom(int chromosomeSize, int minValue, int maxValue, mt19937& rng) {
        for (int i = 0; i < 6; i++) {
            chromosomes[i].initializeRandom(chromosomeSize, minValue, maxValue, rng);
        }
    }
    
    /*
     Obtiene el numero de cromosomas del individuo
     int: Siempre retorna 6 (numero fijo de politicas)
     */
    int getNumChromosomes() const {
        return chromosomes.size();
    }
    
    /*
     Imprime la representacion completa del individuo
     showFitness: Si es true, tambien muestra los valores de fitness
     */
    void print(bool showFitness = false) const {
        printHeader("INDIVIDUO POLIPLOIDE",50);
        
        for (const auto& chromosome : chromosomes) {
            chromosome.print();
        }
        
        if (showFitness) {
            cout << "\nFitness:" << endl;
        }
        
        printDivider(50);
    }
    
    /*
     Valida que el individuo tenga la estructura correcta
     bool: true si es valido, false en caso contrario
     */
    bool isValid() const {
        // Verificar que tenga exactamente 6 cromosomas
        if (chromosomes.size() != 6) {
            return false;
        }
        
        // Verificar que todos los cromosomas tengan el mismo tamaño
        int expectedSize = chromosomes[0].size();
        for (const auto& chromosome : chromosomes) {
            if (chromosome.size() != expectedSize) {
                return false;
            }
            
            // Verificar que todos los genes sean positivos
            for (int gene : chromosome.genes) {
                if (gene <= 0) {
                    return false;
                }
            }
        }
        
        return true;
    }
};



// FUNCIONES DE INICIALIZACIoN DE POBLACIoN

/*
 Calcula el numero total de operaciones en el escenario
 
 Suma todas las operaciones de todos los trabajos para determinar
 el tamaño que debe tener cada cromosoma
 
 data: Datos del escenario cargado
 int: Numero total de operaciones en todos los trabajos
 */
int calculateTotalOperations(const ScenarioData& data) {
    int total = 0;
    for (const auto& job : data.jobs) {
        total += job.operations.size();
    }
    return total;
}

/*
 Inicializa un individuo con valores aleatorios validos
 
 Crea un individuo poliploide donde cada cromosoma contiene genes
 con valores aleatorios en el rango [1, numMachines]. Este rango
 representa las prioridades de maquinas para cada operacion.
 
 data: Datos del escenario (para obtener dimensiones)
 rng: Generador de numeros aleatorios
 Individual: Individuo inicializado con valores aleatorios
 */
Individual initializeIndividualRandom(const ScenarioData& data, mt19937& rng) {
    Individual individual;
    
    // Calcular numero total de operaciones en el escenario
    int totalOperations = calculateTotalOperations(data);
    
    // Los genes pueden tomar valores de 1 a numMachines
    // Estos valores representan prioridades de maquinas
    int minValue = 1;
    int maxValue = data.numMachines;
    
    // Inicializar todos los cromosomas con valores aleatorios
    individual.initializeRandom(totalOperations, minValue, maxValue, rng);
    
    return individual;
}

/*
 Inicializa una poblacion completa de individuos
 
 populationSize: Numero de individuos a crear
 data: Datos del escenario
 rng: Generador de numeros aleatorios
 vector<Individual>: Poblacion inicializada
 */
vector<Individual> initializePopulation(int populationSize, const ScenarioData& data, mt19937& rng) {
    printHeader("INICIALIZANDO POBLACION",50);
    vector<Individual> population;
    population.reserve(populationSize);
    
    cout << "Inicializando poblacion de " << populationSize << " individuos..." << endl;
    
    for (int i = 0; i < populationSize; i++) {
        Individual individual = initializeIndividualRandom(data, rng);
        
        // Validar que el individuo sea correcto
        if (!individual.isValid()) {
            throw runtime_error("ERROR: Individuo generado no es valido");
        }
        
        population.push_back(individual);
    }
    
    cout << "Poblacion inicializada exitosamente" << endl << endl;
    
    return population;
}

string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool isCommentOrEmpty(const string& line) {
    string trimmed = trim(line);
    return trimmed.empty() || trimmed[0] == '#';
}

vector<double> parseLineToDoubles(const string& line) {
    vector<double> values;
    stringstream ss(line);
    double value;
    
    while (ss >> value) {
        values.push_back(value);
    }
    
    return values;
}

vector<int> parseJobOperations(const string& line) {
    vector<int> operations;
    
    size_t start = line.find('{');
    size_t end = line.find('}');
    
    if (start == string::npos || end == string::npos) {
        return operations;
    }
    
    string content = line.substr(start + 1, end - start - 1);
    stringstream ss(content);
    string token;
    
    while (getline(ss, token, ',')) {
        token = trim(token);
        size_t pos = token.find('O');
        if (pos != string::npos && pos + 1 < token.length()) {
            string numStr = token.substr(pos + 1);
            int opNum = stoi(numStr);
            operations.push_back(opNum - 1);
        }
    }
    
    return operations;
}

ScenarioData loadScenario(const string& filename) {
    ScenarioData data;
    ifstream file(filename);
    
    if (!file.is_open()) {
        throw runtime_error("ERROR: No se pudo abrir el archivo: " + filename);
    }
    
    string line;
    int section = 0;
    int rowCount = 0;
    
    data.processingTime.resize(data.numOperations, vector<double>(data.numMachines, 0.0));
    data.energyCost.resize(data.numOperations, vector<double>(data.numMachines, 0.0));

    printSubHeader("CARGANDO ESCENARIO DESDE: "+filename,50);
    
    while (getline(file, line)) {
        if (isCommentOrEmpty(line)) {
            string trimmed = trim(line);
            
            if (trimmed.find("tiempos") != string::npos || 
                trimmed.find("Tiempos") != string::npos) {
                section = 1;
                rowCount = 0;
                printSubHeader("TIEMPOS DE PROCESAMIENTO [Operacion][Maquina]",50);
            }
            else if (trimmed.find("energ") != string::npos || 
                     trimmed.find("Energ") != string::npos) {
                section = 2;
                rowCount = 0;
                printSubHeader("CONSUMO ENERGeTICO [Operacion][Maquina]",50);
            }
            else if (trimmed.find("Trabajo") != string::npos || 
                     trimmed.find("trabajo") != string::npos) {
                section = 3;
                printSubHeader("TRABAJOS Y SUS OPERACIONES",50);
            }
            
            continue;
        }
        
        if (section == 1 && rowCount < data.numOperations) {
            vector<double> times = parseLineToDoubles(line);
            
            if (times.size() != static_cast<size_t>(data.numMachines)) {
                throw runtime_error("ERROR: Fila de tiempos con numero incorrecto de maquinas");
            }
            
            for (int m = 0; m < data.numMachines; m++) {
                data.processingTime[rowCount][m] = times[m];
            }
            
            cout << "Op" << rowCount << ": ";
            for (int m = 0; m < data.numMachines; m++) {
                cout << data.processingTime[rowCount][m] << "\t";
            }
            cout << endl;
            
            rowCount++;
        }
        else if (section == 2 && rowCount < data.numOperations) {
            vector<double> energy = parseLineToDoubles(line);
            
            if (energy.size() != static_cast<size_t>(data.numMachines)) {
                throw runtime_error("ERROR: Fila de energia con numero incorrecto de maquinas");
            }
            
            for (int m = 0; m < data.numMachines; m++) {
                data.energyCost[rowCount][m] = energy[m];
            }
            
            cout << "Op" << rowCount << ": ";
            for (int m = 0; m < data.numMachines; m++) {
                cout << data.energyCost[rowCount][m] << "\t";
            }
            cout << endl;
            
            rowCount++;
        }
        else if (section == 3) {
            vector<int> ops = parseJobOperations(line);
            
            if (!ops.empty()) {
                Job job(data.numJobs);
                job.operations = ops;
                data.jobs.push_back(job);
                
                cout << "Job" << data.numJobs << ": ";
                for (size_t i = 0; i < ops.size(); i++) {
                    cout << "O" << (ops[i] + 1);
                    if (i < ops.size() - 1) cout << " -> ";
                }
                cout << " (" << ops.size() << " operaciones)" << endl;
                
                data.numJobs++;
            }
        }
    }
    
    file.close();
    vector<string> policyNames = {"FIFO", "LTP", "STP", "RRFIFO", "RRLTP", "RRECA"};

    vector<pair<Job, double>> jobsWithTimes;
    vector<pair<Job, double>> jobsWithEnergy;
    for (auto job : data.jobs){
        double totalTime = 0.0;
        double totalEnergy = 0.0;
        for (auto op : job.operations){
            double tempTime = 0.0;
            double tempEnergy = 0.0;
            for (int m = 0; m < data.numMachines; m++){
                tempTime += data.processingTime[op][m];
                tempEnergy += data.energyCost[op][m];
            }
            totalTime += (tempTime/data.numMachines);
            totalEnergy += (tempEnergy/data.numMachines);
        }
        jobsWithTimes.push_back({job, totalTime});
        jobsWithEnergy.push_back({job, totalEnergy});
    } 
    for (const string& policy : policyNames) {
        data.chromosomeMapping[policy] = {};
        cout << "\nMapping para politica: " << policy << "..." << endl;
        vector<queue<pair<Job,Operation>>> roundRobinVector(data.numJobs);
        if (policy == "FIFO"){
            for (auto job : data.jobs){
                for (auto op : job.operations){
                    Operation operation(op);
                    data.chromosomeMapping[policy].push_back({job, operation});
                }
            } 
        }
        else if (policy == "LTP"){
            sort(jobsWithTimes.begin(), jobsWithTimes.end(),
                [](const auto& a, const auto& b) {
                    return a.second > b.second;
                });
            for (const auto& pair : jobsWithTimes) {
                auto job = pair.first;
                for (auto op : pair.first.operations){
                    data.chromosomeMapping[policy].push_back({job, op});
                }
            }
        }
        else if (policy == "STP"){
            sort(jobsWithTimes.begin(), jobsWithTimes.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                });
            for (const auto& pair : jobsWithTimes) {
                auto job = pair.first;
                for (auto op : pair.first.operations){
                    data.chromosomeMapping[policy].push_back({job, op});
                }
            }
        }
        else if(policy == "RRFIFO" || policy == "RRECA" || policy == "RRLTP"){
            if (policy == "RRFIFO"){
                for (int i=0; i < data.numJobs; i++){
                    Job job = data.jobs[i];
                    for (auto op : data.jobs[i].operations){
                        roundRobinVector[i].push({job,op});
                    }
                } 
            }
            else if(policy == "RRLTP"){
                sort(jobsWithTimes.begin(), jobsWithTimes.end(),
                    [](const auto& a, const auto& b) {
                        return a.second > b.second;
                    });
                for (int i=0; i < data.numJobs; i++){
                    Job job = jobsWithTimes[i].first;
                    for (auto op : job.operations){
                        roundRobinVector[i].push({job,op});
                    }
                } 
            }
            else if(policy == "RRECA"){
                sort(jobsWithEnergy.begin(), jobsWithEnergy.end(),
                    [](const auto& a, const auto& b) {
                        return a.second < b.second;
                    });
                for (int i=0; i < data.numJobs; i++){
                    Job job = jobsWithTimes[i].first;
                    for (auto op : job.operations){
                        roundRobinVector[i].push({job,op});
                    }
                } 
            }
            int numJobs = data.numJobs;
            while (!roundRobinVector.empty()){
                for (int i = 0; i < numJobs; i++) {
                    
                    if (!roundRobinVector[i].empty()) {
                        data.chromosomeMapping[policy].push_back(roundRobinVector[i].front());
                        roundRobinVector[i].pop();
                    }
                    else {
                        roundRobinVector.erase(roundRobinVector.begin() + i); 
                        numJobs--;   
                    }
                }
            }
        }
        for (size_t i = 0; i < data.chromosomeMapping[policy].size(); i++){
            cout << "Cromosoma Index: " << i << " -> Operacion: [J"<<data.chromosomeMapping[policy][i].first.id + 1 <<" O" << data.chromosomeMapping[policy][i].second.id +1 <<"]"<< endl;
        }
    }
    


    
    cout << "\nEscenario cargado exitosamente" << endl;
    printDivider(50);
    
    return data;
}

// MoDULO DE EVALUACIoN DE INDIVIDUOS POLIPLOIDES
/*
 Calcula el tiempo de inicio valido para una operacion
 
 Considera dos restricciones fundamentales:
 1. Precedencia de operaciones: La operacion no puede iniciar hasta que
    termine la operacion anterior del mismo trabajo
 2. Disponibilidad de maquina: La operacion no puede iniciar hasta que
    la maquina asignada este libre
 
 El tiempo de inicio es el maximo entre ambas restricciones
 
 machineAvailableTime: Tiempo en que la maquina estara libre
 jobLastOperationTime: Tiempo de finalizacion de la operacion anterior del trabajo
 double: Tiempo de inicio valido para la operacion
 */
double calculateStartTime(double machineAvailableTime, double jobLastOperationTime) {
    // La operacion inicia cuando AMBAS condiciones se cumplan:
    // - La maquina esta libre
    // - La operacion anterior del trabajo termino
    return std::max(machineAvailableTime, jobLastOperationTime);
}
/*
 Programa una operacion en una maquina especifica
 
 Realiza todo el proceso de asignacion de una operacion:
 1. Determina el tiempo de inicio valido (respetando precedencias)
 2. Calcula el tiempo de finalizacion
 3. Actualiza el estado de la maquina
 4. Actualiza el progreso del trabajo
 5. Registra la programacion completa
 
 opId: ID de la operacion a programar
 jobId: ID del trabajo al que pertenece
 machineId: Maquina donde se ejecutara
 machines: Vector de estados de todas las maquinas (se actualiza)
 jobStates: Vector de estados de todos los trabajos (se actualiza)
 data: Datos del escenario (tiempos y costos)
 OperationSchedule: Estructura con toda la informacion de la operacion programada
 */
OperationSchedule scheduleOperation(
    int opId, 
    int jobId, 
    int machineId,
    vector<MachineState>& machines,
    vector<JobState>& jobStates,
    const ScenarioData& data
) {
    OperationSchedule schedule;
    
    // Informacion basica de la operacion
    schedule.operationId = opId;
    schedule.jobId = jobId;
    schedule.machineId = machineId;
    
    // Obtener tiempo de procesamiento y costo energetico de las matrices
    schedule.processingTime = data.processingTime[opId][machineId];
    schedule.energyCost = data.energyCost[opId][machineId];
    
    // Calcular tiempo de inicio respetando:
    // 1. Disponibilidad de la maquina
    // 2. Finalizacion de operacion anterior del trabajo
    schedule.startTime = calculateStartTime(
        machines[machineId].currentTime,
        jobStates[jobId].lastOperationEndTime
    );
    
    // Calcular tiempo de finalizacion
    schedule.endTime = schedule.startTime + schedule.processingTime;
    
    // Actualizar estado de la maquina
    machines[machineId].currentTime = schedule.endTime;
    machines[machineId].totalEnergy += schedule.energyCost;
    machines[machineId].isActive = true;  // Maquina ahora tiene trabajo
    
    // Actualizar estado del trabajo
    jobStates[jobId].lastOperationEndTime = schedule.endTime;
    jobStates[jobId].nextOperationIndex++;
    
    return schedule;
}

/*
 Imprime el scheduling completo de forma legible
 
 util para depuracion y visualizacion del resultado de la simulacion
 
 schedule: Vector con todas las operaciones programadas
 data: Datos del escenario
 */
void printSchedule(const vector<OperationSchedule>& schedule, const ScenarioData& data) {
    cout<<endl;
    printHeader("SCHEDULING COMPLETO",50);
    
    cout << "\nOp\tJob\tMaquina\tInicio\tFin\tTiempo\tEnergia" << endl;
    cout << "--------------------------------------------------" << endl;
    
    for (const auto& op : schedule) {
        cout << "O" << (op.operationId + 1) << "\t"
             << "J" << op.jobId << "\t"
             << "M" << op.machineId << "\t"
             << op.startTime << "\t"
             << op.endTime << "\t"
             << op.processingTime << "\t"
             << op.energyCost << endl;
    }
    
    printDivider(50);
}

bool evaluateChromosome(
    Chromosome& chromosome,
    const ScenarioData& data,
    bool printScheduleFlag = false
) {
    
    // Validar tamaño del cromosoma
    int totalOperations = calculateTotalOperations(data);
    if (chromosome.size() != totalOperations) {
        cerr << "ERROR: Tamaño de cromosoma no coincide con numero de operaciones" << endl;
        return false;
    }
    
    // Vector para almacenar todas las operaciones programadas
    vector<OperationSchedule> schedule;
    string policy = chromosome.policyName;
    
    // Inicializar estados de todas las maquinas
    vector<MachineState> machines(data.numMachines);
    
    // Inicializar estados de todos los trabajos
    vector<JobState> jobStates(data.numJobs);

    for (int i = 0; i< chromosome.genes.size(); i++){
        int operationId = data.chromosomeMapping.at(policy)[i].second.id;
        int jobId = data.chromosomeMapping.at(policy)[i].first.id;
        int machineId = chromosome.genes[i] - 1;
        // Programar la operacion en la maquina seleccionada
        OperationSchedule opSchedule = scheduleOperation(
            operationId,
            jobId,
            machineId,
            machines,
            jobStates,
            data
        );
        
        // Guardar la operacion programada
        schedule.push_back(opSchedule);
    }
    double makespan = 0.0;
    double totalEnergy = 0.0;
    for (const auto& machine : machines) {
        if (machine.isActive) {
            if (machine.currentTime > makespan) {
                makespan = machine.currentTime;
            }
            totalEnergy += machine.totalEnergy;
        }
    }
    
    chromosome.f1 = makespan;
    chromosome.f2 = totalEnergy;

    if (printScheduleFlag) {
        printSchedule(schedule, data);
    }
    
    return true;
}

/*
 Evalua todas las politicas de un individuo
 
 Ejecuta la evaluacion para los 6 cromosomas del individuo y
 muestra un resumen comparativo de todas las politicas
 
 individual: Individuo a evaluar completamente
 data: Datos del escenario
 */
void evaluateAllPolicies(Individual& individual, const ScenarioData& data, string individuo, bool showTable = false, bool showSchedule = false) {
    
    for (int i = 0; i < individual.getNumChromosomes(); i++) {
        bool success = evaluateChromosome(individual.chromosomes[i], data, showSchedule);
        if (!success) {
            cerr << "ERROR: Evaluacion de politica fallo" << endl;
            continue;
        }  
    }    
    vector<string> fields = {"Politica", "Makespan", "Energia"};
    vector<vector<string>> values;

    for (int i = 0; i < individual.getNumChromosomes(); i++) {
        vector<string> row;
        row.push_back(individual.chromosomes[i].policyName);
        row.push_back(to_string(individual.chromosomes[i].f1));
        row.push_back(to_string(individual.chromosomes[i].f2));
        values.push_back(row);
    }
    if (showTable){
        cout<<endl;
        printHeader("EVALUACION POLITICAS DEL INDIVIDUO: "+ individuo,50);
        individual.print(false);
        printTable(fields, values);
    }
    
}

void calculateCrowdingDistanceChromosome(vector<Individual*>& front, int chromosomeIndex) {
    int size = front.size();
    if (size == 0) return;

    for (auto& ind : front) {
        ind->chromosomes[chromosomeIndex].crowdingDistance = 0;
    }

    auto compareF1 = [chromosomeIndex](Individual* a, Individual* b) {
        return a->chromosomes[chromosomeIndex].f1 < b->chromosomes[chromosomeIndex].f1;
    };
    auto compareF2 = [chromosomeIndex](Individual* a, Individual* b) {
        return a->chromosomes[chromosomeIndex].f2 < b->chromosomes[chromosomeIndex].f2;
    };

    sort(front.begin(), front.end(), compareF1);
    front[0]->chromosomes[chromosomeIndex].crowdingDistance = numeric_limits<double>::infinity();
    front[size - 1]->chromosomes[chromosomeIndex].crowdingDistance = numeric_limits<double>::infinity();

    double f1Min = front[0]->chromosomes[chromosomeIndex].f1;
    double f1Max = front[size - 1]->chromosomes[chromosomeIndex].f1;

    for (int i = 1; i < size - 1; i++) {
        if (f1Max - f1Min == 0) continue;
        front[i]->chromosomes[chromosomeIndex].crowdingDistance +=
            (front[i + 1]->chromosomes[chromosomeIndex].f1 - front[i - 1]->chromosomes[chromosomeIndex].f1) / (f1Max - f1Min);
    }

    sort(front.begin(), front.end(), compareF2);
    front[0]->chromosomes[chromosomeIndex].crowdingDistance = numeric_limits<double>::infinity();
    front[size - 1]->chromosomes[chromosomeIndex].crowdingDistance = numeric_limits<double>::infinity();

    double f2Min = front[0]->chromosomes[chromosomeIndex].f2;
    double f2Max = front[size - 1]->chromosomes[chromosomeIndex].f2;

    for (int i = 1; i < size - 1; i++) {
        if (f2Max - f2Min == 0) continue;
        front[i]->chromosomes[chromosomeIndex].crowdingDistance +=
            (front[i + 1]->chromosomes[chromosomeIndex].f2 - front[i - 1]->chromosomes[chromosomeIndex].f2) / (f2Max - f2Min);
    }
}

void fastNonDominatedSort(vector<Individual>& population) {
    for (int c = 0; c < population[0].getNumChromosomes();c++){
        int rank = 0;
        vector<Individual*> dominatedIndividuals;
        vector<Individual*> dominatedIndividualsTemp;
        for (auto& ind : population) {
            dominatedIndividuals.push_back(&ind); 
        }
        while (!dominatedIndividuals.empty()) {
            rank++;
            for (size_t i = 0; i < dominatedIndividuals.size(); ++i) {
                bool isDominated = false;
                for (size_t j = 0; j < dominatedIndividuals.size(); ++j) {
                    if (i == j) continue;
                    auto& A = dominatedIndividuals[j]; // posible dominante
                    auto& B = dominatedIndividuals[i]; // posible dominado

                    bool betterOrEqualInAll =
                        A->chromosomes[c].f1 <= B->chromosomes[c].f1 &&
                        A->chromosomes[c].f2 <= B->chromosomes[c].f2;

                    bool strictlyBetterInAtLeastOne =
                        A->chromosomes[c].f1 < B->chromosomes[c].f1 ||
                        A->chromosomes[c].f2 < B->chromosomes[c].f2;

                    if (betterOrEqualInAll && strictlyBetterInAtLeastOne) {
                        isDominated = true;
                        break;
                    }
                }

                if (isDominated)
                    dominatedIndividualsTemp.push_back(dominatedIndividuals[i]);
                else
                    dominatedIndividuals[i]->chromosomes[c].domLevel = rank;
            }
            dominatedIndividuals = dominatedIndividualsTemp;
            dominatedIndividualsTemp.clear();
        }
        for (int r = 1; r <= rank; r++) {
            vector<Individual*> front;
            for (auto& ind : population) {
                if (ind.chromosomes[c].domLevel == r) {
                    front.push_back(&ind);
                }
            }
            calculateCrowdingDistanceChromosome(front, c);
        }
    }
}

Individual tournamentSelection(const vector<Individual>& population) {
    int i = rand() % population.size();
    int j = rand() % population.size();
    
    const Individual& A = population[i];
    const Individual& B = population[j];
    
    // Primero se mira el frente de Pareto
    int rankAggregateA = 0;
    int rankAggregateB = 0;
    for (auto& chr : A.chromosomes)
        rankAggregateA += chr.domLevel;
    for (auto& chr : B.chromosomes)
        rankAggregateB += chr.domLevel;
    if (rankAggregateA < rankAggregateB)
        return A;
    else if (rankAggregateB < rankAggregateA)
        return B;
    
    // Si están en el mismo frente, mirar crowding distance
    int crowdingAggregateA = 0;
    int crowdingAggregateB = 0;
    for (auto& chr : A.chromosomes)
        crowdingAggregateA += chr.crowdingDistance;
    for (auto& chr : B.chromosomes)
        crowdingAggregateB += chr.crowdingDistance;
    if (crowdingAggregateA > crowdingAggregateB)
        return A;
    else
        return B;
}

vector<Individual> selectParents(const vector<Individual>& population, int numParents) {
    vector<Individual> parents;
    parents.reserve(numParents);
    
    for (int i = 0; i < numParents; i++) {
        Individual parent = tournamentSelection(population);
        parents.push_back(parent);
    }
    return parents;
}

vector<Individual> uniformCrossover(const Individual& parent1, const Individual& parent2, mt19937& rng, float crossoverRate,  uniform_real_distribution<double>& dist) {
    Individual offspring1;
    Individual offspring2;
    
    if (dist(rng) < crossoverRate){
        for (int i = 0; i < parent1.chromosomes[0].genes.size(); i++) {
            if (dist(rng) < 0.5){
                for(int j = 0; j < parent1.getNumChromosomes(); j++){
                    offspring1.chromosomes[j].genes.push_back(parent1.chromosomes[j].genes[i]);
                    offspring2.chromosomes[j].genes.push_back(parent2.chromosomes[j].genes[i]);
                }
            } else {
                for(int j = 0; j < parent1.getNumChromosomes(); j++){
                    offspring1.chromosomes[j].genes.push_back(parent2.chromosomes[j].genes[i]);
                    offspring2.chromosomes[j].genes.push_back(parent1.chromosomes[j].genes[i]);
                }
            }
        }
    } else {
        offspring1 = parent1;
        offspring2 = parent2;
    }
    return {offspring1, offspring2};
}

vector<Individual> uniformCrossoverPopulation(const vector<Individual>& parents, mt19937& rng, float crossoverRate, uniform_real_distribution<double> dist) {
    vector<Individual> offspring;
    offspring.reserve(parents.size());
    
    for (size_t i = 0; i < parents.size(); i += 2) {
        const Individual& parent1 = parents[i];
        const Individual& parent2 = parents[(i + 1) % parents.size()];
        
        vector<Individual> children = uniformCrossover(parent1, parent2, rng, crossoverRate, dist);
        offspring.push_back(children[0]);
        offspring.push_back(children[1]);
    }
    
    return offspring;
}

vector<Individual> selectSurvivors(const vector<Individual>& combinedPopulation, int desiredSize) {
    vector<Individual> newPopulation;
    newPopulation.reserve(desiredSize);
 
    for(int i = 0; i<desiredSize; i++){
        int index1 = rand() % combinedPopulation.size();
        int index2 = rand() % combinedPopulation.size();
        const Individual& A = combinedPopulation[index1];
        const Individual& B = combinedPopulation[index2];
        Individual superIndividual;
        for(int c=0; c<A.getNumChromosomes(); c++){
            if (A.chromosomes[c].domLevel < B.chromosomes[c].domLevel){
                for(int j=0; j<A.chromosomes[c].genes.size(); j++){
                    superIndividual.chromosomes[c].genes.push_back(A.chromosomes[c].genes[j]);
                }
            }
            else if (B.chromosomes[c].domLevel < A.chromosomes[c].domLevel){
                for(int j=0; j<B.chromosomes[c].genes.size(); j++){
                    superIndividual.chromosomes[c].genes.push_back(B.chromosomes[c].genes[j]);
                }
            }
            else {
                if (A.chromosomes[c].crowdingDistance > B.chromosomes[c].crowdingDistance){
                    for(int j=0; j<A.chromosomes[c].genes.size(); j++){
                        superIndividual.chromosomes[c].genes.push_back(A.chromosomes[c].genes[j]);
                    }
                } else {
                    for(int j=0; j<B.chromosomes[c].genes.size(); j++){
                        superIndividual.chromosomes[c].genes.push_back(B.chromosomes[c].genes[j]);
                    }
                }
            }
        }
        newPopulation.push_back(superIndividual);
    }
    return newPopulation;
}

void mutationInterChromosome(Individual& individual, mt19937& rng, float mutationRate, uniform_real_distribution<double>& dist) {
    if (dist(rng) < mutationRate){
        uniform_int_distribution<int> distInt(0, individual.getNumChromosomes() - 1);
        int a = distInt(rng);
        int b;
        do {
            b = distInt(rng);
        } while (b == a);
        swap(individual.chromosomes[a].genes, individual.chromosomes[b].genes);
    }
}

void mutationReciprocalExchange(Individual& individual, mt19937& rng, float mutationRate, uniform_real_distribution<double>& dist) {
    if (dist(rng) < mutationRate){
        uniform_int_distribution<int> distK(1, 3); // rango [1,3]
        int n = individual.chromosomes[0].genes.size();
        for (int c = 0; c < individual.getNumChromosomes(); c++){
            int k = distK(rng);
            vector<int> indexs(n);
            iota(indexs.begin(), indexs.end(), 0);  // 0..n-1
            shuffle(indexs.begin(), indexs.end(), rng);

            for (int pairCount = 0; pairCount < k; pairCount++) {
                int i = indexs[2*pairCount];
                int j = indexs[2*pairCount+1];
                swap(individual.chromosomes[c].genes[i], individual.chromosomes[c].genes[j]);
            }
        }
    }
}

void mutationShift(Individual& individual, mt19937& rng, float mutationRate, uniform_real_distribution<double>& dist){
    if (dist(rng) < mutationRate){
        uniform_int_distribution<int> distWindow(3, 5); 
        int windowSize = distWindow(rng);
        int n = individual.chromosomes[0].genes.size();
        for (int c = 0; c < individual.getNumChromosomes(); c++){
            uniform_int_distribution<int> distStart(0, n - windowSize);
            int startIdx = distStart(rng);
            vector<int> windowGenes;
            for (int i = startIdx; i < startIdx + windowSize; i++) {
                windowGenes.push_back(individual.chromosomes[c].genes[i]);
            }
            // Shift right
            for (int i = startIdx + windowSize - 1; i > startIdx; i--) {
                individual.chromosomes[c].genes[i] = individual.chromosomes[c].genes[i - 1];
            }
            individual.chromosomes[c].genes[startIdx] = windowGenes.back();
        }
    }
}

void graphParetoFront(const vector<Individual>& population, int chromosomeIndex) {
    vector<double> x_vals;
    vector<double> y_vals;
    vector<double> c; 
    int maxDomLevel = INT_MIN;
    for (const Individual& ind : population) {
        int level = ind.chromosomes[chromosomeIndex].domLevel;
        if (level > maxDomLevel)
            maxDomLevel = level;
    }
    for (int level = 1; level <= maxDomLevel; level++) {
        vector<Individual> front;
        for (const Individual& ind : population) {
            if (ind.chromosomes[chromosomeIndex].domLevel == level) {
                x_vals.push_back(ind.chromosomes[chromosomeIndex].f1);
                y_vals.push_back(ind.chromosomes[chromosomeIndex].f2);
                c.push_back(level);
            }
        }
    }
    scatter(x_vals, y_vals, 10, c);
    title("Poblacion - Distribucion de Makespan vs Energia");
    xlabel("Makespan");
    ylabel("Energia");
    show();
}

vector<Individual> geneticAlgorithmStep(vector<Individual>& population, const ScenarioData& scenario, int populationSize, mt19937& rng) {
    uniform_real_distribution<double> dist(0.0, 1.0);
    
    vector<Individual> parents = selectParents(population, populationSize);
    vector<Individual> offspring = uniformCrossoverPopulation(parents, rng, 0.9, dist);
    
    for (size_t i = 0; i < offspring.size(); i++){
        string individuo = to_string(i+1);
        evaluateAllPolicies(offspring[i], scenario, individuo, false, false);
    }
    
    vector<Individual> populationWithOffspring = population;
    populationWithOffspring.insert(populationWithOffspring.end(), offspring.begin(), offspring.end());
    for (int i=0; i<populationWithOffspring.size(); i++){
        mutationInterChromosome(populationWithOffspring[i], rng, 0.3, dist);
        mutationReciprocalExchange(populationWithOffspring[i], rng, 0.2, dist);
        mutationShift(populationWithOffspring[i], rng, 0.1, dist);
        for (int j=0; j<populationWithOffspring[i].chromosomes.size(); j++){
            populationWithOffspring[i].chromosomes[j].domLevel = -1;
            populationWithOffspring[i].chromosomes[j].crowdingDistance = -1;
        }
    }
    fastNonDominatedSort(populationWithOffspring);
    population = selectSurvivors(populationWithOffspring, populationSize);
    for (int i=0; i<population.size(); i++){
        for (int j=0; j<population[i].chromosomes.size(); j++){
            population[i].chromosomes[j].domLevel = -1;
            population[i].chromosomes[j].crowdingDistance = -1;
        }
        evaluateAllPolicies(population[i], scenario, to_string(i+1), false, false);
    }
    fastNonDominatedSort(population);
    return population;
}

double calculateHyperVolume(const vector<Individual>& population, int chromosomeIndex, double refPointF1, double refPointF2) {
    vector<pair<double, double>> points;
    for (const auto& ind : population) {
        if (ind.chromosomes[chromosomeIndex].domLevel == 1)
            points.push_back({ind.chromosomes[chromosomeIndex].f1, ind.chromosomes[chromosomeIndex].f2});
    }
    sort(points.begin(), points.end());
    
    double hypervolume = 0.0;
    double prevF1 = refPointF1;
    
    for (const auto& point : points) {
        double f1 = point.first;
        double f2 = point.second;
        
        if (f1 < refPointF1 && f2 < refPointF2) {
            hypervolume += (prevF1 - f1) * (refPointF2 - f2);
            prevF1 = f1;
        }
    }
    return hypervolume;
}

int main() {
    try {
        int populationSize = 20;
        int numGenerations = 100;
        mt19937 rng(time(nullptr));

        string filename = "escenario1.txt";
        cout<<endl;
        printHeader("ALGORITMO GENETICO POLIPLOIDE",60);
        
        // Cargar escenario
        ScenarioData scenario = loadScenario(filename);
        
        // Calcular dimensiones
        int totalOps = calculateTotalOperations(scenario);
        cout << "Total de operaciones en el escenario: " << totalOps << endl;
        cout << "Numero de maquinas: " << scenario.numMachines << endl;
        cout << "Rango de genes: [1, " << scenario.numMachines << "]" << endl << endl;
        
        // Crear una poblacion pequeña
        
        vector<Individual> population = initializePopulation(populationSize, scenario, rng);

        printSubHeader("RESUMEN DE POBLACION INICIAL",50);
        cout << "Tamano de poblacion: " << population.size() << endl;
        cout << "Cromosomas por individuo: " << population[0].getNumChromosomes() << endl;
        cout << "Genes por cromosoma: " << population[0].chromosomes[0].size() << endl;

        


        // FIFO
        vector<double> x_vals;
        vector<double> y_vals;
        vector<double> c; 

        for (size_t i = 0; i < population.size(); i++){
            string individuo = to_string(i+1);
            evaluateAllPolicies(population[i], scenario, individuo, false, false);
            for (size_t j = 0; j < population[i].chromosomes.size(); j++) {
                x_vals.push_back(population[i].chromosomes[j].f1);
                y_vals.push_back(population[i].chromosomes[j].f2);
                c.push_back(j);
            }

        }
        scatter(x_vals, y_vals, 5, c);
        title("Poblacion Inicial - Distribucion de Makespan vs Energia");
        xlabel("Makespan");
        ylabel("Energia");
        show();

        double f1_max = population[0].chromosomes[0].f1;
        double f2_max = population[0].chromosomes[0].f2;

        for (auto& ind : population) {
            for (auto& chrom : ind.chromosomes) {
                if (chrom.f1 > f1_max) f1_max = chrom.f1;
                if (chrom.f2 > f2_max) f2_max = chrom.f2;
            }
        }

        f1_max += 50;
        f2_max += 50;
        fastNonDominatedSort(population);
        vector<vector<double>> hypervolumes(population[0].getNumChromosomes());
        for(int gen = 1; gen < numGenerations+1; gen++){
            population = geneticAlgorithmStep(population, scenario, populationSize, rng);
            for (int i=0; i<population[0].getNumChromosomes(); i++){
                double hv = calculateHyperVolume(population, i, f1_max, f2_max);
                hypervolumes[i].push_back(hv);
            }
            if (gen % 20 == 0){
                printHeader("GENERACION " + to_string(gen), 50);
                vector<vector<string>> hvTableValues;
                vector<string> hvTableFields = {"Politica", "Min", "Max", "Promedio"};
                for (int i=0; i<population[0].getNumChromosomes(); i++){
                    int minVal = *min_element(hypervolumes[i].begin(), hypervolumes[i].end());
                    int maxVal = *max_element(hypervolumes[i].begin(), hypervolumes[i].end());
                    double mean = accumulate(hypervolumes[i].begin(), hypervolumes[i].end(), 0.0) / hypervolumes[i].size();
                    vector<string> row;
                    row.push_back(population[0].chromosomes[i].policyName);
                    row.push_back(to_string(minVal));
                    row.push_back(to_string(maxVal));
                    row.push_back(to_string(mean));
                    hvTableValues.push_back(row);
                }
                printTable(hvTableFields, hvTableValues);
            }
        }
        for (size_t i = 0; i < population.size(); i++){
            for (size_t j = 0; j < population[i].chromosomes.size(); j++) {
                x_vals.push_back(population[i].chromosomes[j].f1);
                y_vals.push_back(population[i].chromosomes[j].f2);
                c.push_back(j);
            }

        }
        scatter(x_vals, y_vals, 5, c);
        title("Poblacion Inicial - Distribucion de Makespan vs Energia");
        xlabel("Makespan");
        ylabel("Energia");
        show();
        
    } catch (const exception& e) {
        cerr << "EXCEPCION: " << e.what() << endl;
        return 1;
    }
    return 0;
}