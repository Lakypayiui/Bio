#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

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
    char topLeft, topRight, bottomLeft, bottomRight, horizontal, vertical;
        topLeft = 201;  // ╔
        topRight = 187; // ╗
        bottomLeft = 200; // ╚
        bottomRight = 188; // ╝
        horizontal = 205; // ═
        vertical = 186; // ║
    
    #else
    string topLeft, topRight, bottomLeft, bottomRight, horizontal, vertical;
        topLeft = "╔";
        topRight = "╗";
        bottomLeft = "╚";
        bottomRight = "╝";
        horizontal = "═";
        vertical = "║";
        

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
        cout << bottomRight << endl;
}

class Chromosome {
public:
    string policyName;
    vector<int> genes;
    
    // Constructor por defecto
    Chromosome() : policyName("") {}
    
    // Constructor con nombre de politica
    Chromosome(const string& name) : policyName(name) {}
    
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
    double f1;  // Makespan
    double f2;  // Energia total
    
    // Constructor por defecto
    Individual() : f1(0.0), f2(0.0) {
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
        
        // Resetear valores de fitness
        f1 = 0.0;
        f2 = 0.0;
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
            cout << "  f1 (Makespan): " << f1 << endl;
            cout << "  f2 (Energia): " << f2 << endl;
        }
        
        cout << "======================================================" << endl;
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
    
    cout << "   Poblacion inicializada exitosamente" << endl;
    
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
    
    cout << "=== CARGANDO ESCENARIO DESDE: " << filename << " ===" << endl << endl;
    
    while (getline(file, line)) {
        if (isCommentOrEmpty(line)) {
            string trimmed = trim(line);
            
            if (trimmed.find("tiempos") != string::npos || 
                trimmed.find("Tiempos") != string::npos) {
                section = 1;
                rowCount = 0;
                cout << "=== TIEMPOS DE PROCESAMIENTO [Operacion][Maquina] ===" << endl;
            }
            else if (trimmed.find("energ") != string::npos || 
                     trimmed.find("Energ") != string::npos) {
                section = 2;
                rowCount = 0;
                cout << "\n=== CONSUMO ENERGeTICO [Operacion][Maquina] ===" << endl;
            }
            else if (trimmed.find("Trabajo") != string::npos || 
                     trimmed.find("trabajo") != string::npos) {
                section = 3;
                cout << "\n=== TRABAJOS Y SUS OPERACIONES ===" << endl;
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
    
    cout << "\n   Escenario cargado exitosamente" << endl;
    cout << "======================================" << endl << endl;
    
    return data;
}

bool validateScenario(const ScenarioData& data) {
    cout << "=== VALIDANDO ESCENARIO ===" << endl;
    
    bool valid = true;
    
    if (data.numJobs == 0) {
        cout << "   ERROR: No se cargaron trabajos" << endl;
        valid = false;
    }
    
    for (const auto& job : data.jobs) {
        for (int opId : job.operations) {
            if (opId < 0 || opId >= data.numOperations) {
                cout << "   ERROR: Job" << job.id << " contiene operacion invalida: O" << (opId + 1) << endl;
                valid = false;
            }
        }
    }
    
    for (int op = 0; op < data.numOperations; op++) {
        for (int m = 0; m < data.numMachines; m++) {
            if (data.processingTime[op][m] < 0) {
                cout << "   ERROR: Tiempo negativo en Op" << op << ", M" << m << endl;
                valid = false;
            }
            if (data.processingTime[op][m] == 0) {
                cout << "⚠ ADVERTENCIA: Tiempo cero en Op" << op << ", M" << m << endl;
            }
        }
    }
    
    for (int op = 0; op < data.numOperations; op++) {
        for (int m = 0; m < data.numMachines; m++) {
            if (data.energyCost[op][m] < 0) {
                cout << "   ERROR: Consumo energetico negativo en Op" << op << ", M" << m << endl;
                valid = false;
            }
        }
    }
    
    if (valid) {
        cout << "   Todos los datos son validos" << endl;
    }
    cout << "======================================" << endl << endl;
    
    return valid;
}
// MoDULO DE EVALUACIoN DE INDIVIDUOS POLIPLOIDES
/*
 Selecciona la maquina optima para una operacion segun el gen del cromosoma
 
 El gen codifica una prioridad que se usa para seleccionar entre las maquinas
 disponibles. El algoritmo:
 1. Obtiene el valor del gen (prioridad en rango [1, numMachines])
 2. Ajusta el indice para que sea valido: (gen - 1) % numMachines
 3. Retorna el ID de maquina correspondiente
 
 gene: Valor del gen del cromosoma (prioridad codificada)
 numMachines: Numero total de maquinas disponibles
 int: ID de la maquina seleccionada [0, numMachines-1]
 */
int selectMachineFromGene(int gene, int numMachines) {
    // El gen debe estar en rango [1, numMachines]
    // Convertimos a indice [0, numMachines-1]
    int machineId = (gene - 1) % numMachines;
    
    // Asegurar que el resultado sea valido
    if (machineId < 0) machineId = 0;
    if (machineId >= numMachines) machineId = numMachines - 1;
    
    return machineId;
}
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
    return max(machineAvailableTime, jobLastOperationTime);
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
 Calcula el makespan (f1) del scheduling completo
 
 El makespan es el tiempo total desde el inicio hasta que todas las
 operaciones han finalizado. Es el maximo tiempo de finalizacion entre
 todas las maquinas activas.
 
 machines: Vector con el estado final de todas las maquinas
 double: Makespan total (tiempo de finalizacion del ultimo trabajo)
 */
double calculateMakespan(const vector<MachineState>& machines) {
    double makespan = 0.0;
    
    // El makespan es el tiempo maximo entre todas las maquinas
    for (const auto& machine : machines) {
        if (machine.isActive) {  // Solo considerar maquinas con trabajo
            makespan = max(makespan, machine.currentTime);
        }
    }
    
    return makespan;
}
/*
 Calcula el consumo energetico total (f2)
 
 Suma el consumo energetico de todas las maquinas que ejecutaron
 al menos una operacion. Las maquinas inactivas no consumen energia.
 
 machines: Vector con el estado final de todas las maquinas
 double: Energia total consumida por todas las maquinas activas
 */
double calculateTotalEnergy(const vector<MachineState>& machines) {
    double totalEnergy = 0.0;
    
    // Sumar energia solo de maquinas activas
    for (const auto& machine : machines) {
        if (machine.isActive) {  // Maquinas vacias no consumen energia
            totalEnergy += machine.totalEnergy;
        }
    }
    
    return totalEnergy;
}
/*
 Simula el scheduling completo de un cromosoma especifico
 
 Este es el nucleo del proceso de evaluacion. Simula la ejecucion
 de todas las operaciones siguiendo:
 1. El orden de precedencia dentro de cada trabajo
 2. Las asignaciones de maquinas codificadas en el cromosoma
 
 Proceso:
 - Inicializa estados de maquinas y trabajos
 - Procesa operaciones trabajo por trabajo, en orden
 - Respeta precedencias (operaciones de un trabajo son secuenciales)
 - Asigna maquinas segun los genes del cromosoma
 - Retorna el vector completo de operaciones programadas
 
 chromosome: Cromosoma que codifica las asignaciones de maquinas
 data: Datos del escenario (trabajos, tiempos, energias)
 vector<OperationSchedule>: Scheduling completo con todas las operaciones
 */
vector<OperationSchedule> simulateScheduling(
    const Chromosome& chromosome,
    const ScenarioData& data
) {
    // Vector para almacenar todas las operaciones programadas
    vector<OperationSchedule> schedule;
    
    // Inicializar estados de todas las maquinas
    vector<MachineState> machines(data.numMachines);
    
    // Inicializar estados de todos los trabajos
    vector<JobState> jobStates(data.numJobs);
    
    // indice global en el cromosoma para acceder a los genes
    int geneIndex = 0;
    
    // Procesar cada trabajo en orden
    for (int j = 0; j < data.numJobs; j++) {
        const Job& job = data.jobs[j];
        
        // Procesar cada operacion del trabajo en orden secuencial
        // IMPORTANTE: Las operaciones de un trabajo deben ejecutarse en orden
        for (int opIdx = 0; opIdx < (int)job.operations.size(); opIdx++) {
            int operationId = job.operations[opIdx];
            
            // Obtener el gen correspondiente a esta operacion
            int gene = chromosome.genes[geneIndex];
            geneIndex++;
            
            // Seleccionar maquina segun el gen
            int machineId = selectMachineFromGene(gene, data.numMachines);
            
            // Programar la operacion en la maquina seleccionada
            OperationSchedule opSchedule = scheduleOperation(
                operationId,
                j,
                machineId,
                machines,
                jobStates,
                data
            );
            
            // Guardar la operacion programada
            schedule.push_back(opSchedule);
        }
    }
    
    return schedule;
}
/*
 FUNCIoN PRINCIPAL DE EVALUACIoN
 
 Evalua un individuo completo ejecutando una de sus politicas (cromosomas)
 
 Proceso completo:
 1. Valida entradas (individuo, indice de cromosoma)
 2. Extrae el cromosoma a evaluar
 3. Simula el scheduling completo segun ese cromosoma
 4. Calcula f1 (makespan)
 5. Calcula f2 (energia total)
 6. Actualiza los valores de fitness del individuo
 
 NOTA: Esta funcion modifica el individuo pasado por referencia,
       actualizando sus valores f1 y f2
 
 individual: Individuo a evaluar (se modifican f1 y f2)
 data: Datos del escenario cargado
 chromosomeIndex: indice del cromosoma/politica a evaluar [0-5]
                  0=FIFO, 1=LTP, 2=STP, 3=RRFIFO, 4=RRLTP, 5=RRECA
 bool: true si la evaluacion fue exitosa, false si hubo error
 */
bool evaluateIndividual(
    Individual& individual,
    const ScenarioData& data,
    int chromosomeIndex
) {
    // Validar indice de cromosoma
    if (chromosomeIndex < 0 || chromosomeIndex >= individual.getNumChromosomes()) {
        cerr << "ERROR: indice de cromosoma invalido: " << chromosomeIndex << endl;
        return false;
    }
    
    // Validar que el individuo sea correcto
    if (!individual.isValid()) {
        cerr << "ERROR: Individuo no valido para evaluacion" << endl;
        return false;
    }
    
    // Obtener el cromosoma a evaluar
    const Chromosome& chromosome = individual.chromosomes[chromosomeIndex];
    
    // Validar tamaño del cromosoma
    int totalOperations = calculateTotalOperations(data);
    if (chromosome.size() != totalOperations) {
        cerr << "ERROR: Tamaño de cromosoma no coincide con numero de operaciones" << endl;
        return false;
    }
    
    // SIMULAR SCHEDULING COMPLETO
    vector<OperationSchedule> schedule = simulateScheduling(chromosome, data);
    
    // Crear estados de maquinas para calculo de metricas
    vector<MachineState> machines(data.numMachines);
    
    // Reconstruir estados finales de maquinas desde el schedule
    for (const auto& op : schedule) {
        machines[op.machineId].currentTime = max(
            machines[op.machineId].currentTime, 
            op.endTime
        );
        machines[op.machineId].totalEnergy += op.energyCost;
        machines[op.machineId].isActive = true;
    }
    
    // CALCULAR MAKESPAN (f1)
    individual.f1 = calculateMakespan(machines);
    
    // ALCULAR ENERGiA TOTAL (f2)
    individual.f2 = calculateTotalEnergy(machines);
    
    return true;
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
    
    cout << "======================================================" << endl;
}
/*
 Evalua todas las politicas de un individuo
 
 Ejecuta la evaluacion para los 6 cromosomas del individuo y
 muestra un resumen comparativo de todas las politicas
 
 individual: Individuo a evaluar completamente
 data: Datos del escenario
 */
void evaluateAllPolicies(Individual& individual, const ScenarioData& data, string individuo) {
    cout << "\n=== EVALUANDO TODAS LAS POLiTICAS DEL INDIVIDUO: "<<individuo<< "===" << endl;
    cout<<endl;
    printHeader("EVALUACION DE TODAS LAS POLITICAS",50);
    
    vector<pair<double, double>> results;  // (f1, f2) para cada politica
    
    for (int i = 0; i < individual.getNumChromosomes(); i++) {
        cout << "\nEvaluando politica: " << individual.chromosomes[i].policyName << "..." << endl;
        
        bool success = evaluateIndividual(individual, data, i);
        
        if (success) {
            results.push_back({individual.f1, individual.f2});
            cout << "  Makespan (f1): " << individual.f1 << endl;
            cout << "  Energia (f2): " << individual.f2 << endl;
        } else {
            cout << "  ERROR en evaluacion" << endl;
            results.push_back({-1.0, -1.0});
        }
    }
    cout<<endl;
    printHeader("RESUMEN COMPARATIVO",50);
    
    cout << "\nPolitica\t\tMakespan\tEnergia" << endl;
    cout << "--------------------------------------------------" << endl;
    
    for (int i = 0; i < individual.getNumChromosomes(); i++) {
        cout << individual.chromosomes[i].policyName << "\t\t"
             << results[i].first << "\t\t"
             << results[i].second << endl;
    }
    
    cout << "======================================================" << endl;
}

int main() {
    try {
        string filename = "escenario1.txt";
        cout<<endl;
        printHeader("ALGORITMO GENETICO POLIPLOIDE",50);
        
        // Cargar escenario
        ScenarioData scenario = loadScenario(filename);
        
        // Validar datos
        if (!validateScenario(scenario)) {
            cerr << "ERROR: El escenario contiene errores" << endl;
            return 1;
        }
        
        // Inicializar generador de numeros aleatorios
        mt19937 rng(time(nullptr));
        
        cout << "\n=== CREANDO INDIVIDUO POLIPLOIDE ===" << endl;
        
        // Calcular dimensiones
        int totalOps = calculateTotalOperations(scenario);
        cout << "Total de operaciones en el escenario: " << totalOps << endl;
        cout << "Numero de maquinas: " << scenario.numMachines << endl;
        cout << "Rango de genes: [1, " << scenario.numMachines << "]" << endl << endl;
        
        // Crear un individuo de ejemplo
        Individual ind1 = initializeIndividualRandom(scenario, rng);
        
        cout << "\n=== INDIVIDUO 1 (ALEATORIO) ===" << endl;
        ind1.print(false);
        
        // Validar individuo
        cout << "\nValidando individuo..." << endl;
        if (ind1.isValid()) {
            cout << "   Individuo valido" << endl;
        } else {
            cout << "   Individuo invalido" << endl;
        }
        
        // Crear otro individuo para demostracion
        cout << "\n=== INDIVIDUO 2 (ALEATORIO) ===" << endl;
        Individual ind2 = initializeIndividualRandom(scenario, rng);
        ind2.print(false);
        
        // Crear una poblacion pequeña
        cout << "\n=== INICIALIZANDO POBLACIoN ===" << endl;
        int populationSize = 5;
        vector<Individual> population = initializePopulation(populationSize, scenario, rng);
        
        cout << "\n=== RESUMEN DE LA POBLACIoN ===" << endl;
        cout << "Tamaño de poblacion: " << population.size() << endl;
        cout << "Cromosomas por individuo: " << population[0].getNumChromosomes() << endl;
        cout << "Genes por cromosoma: " << population[0].chromosomes[0].size() << endl;
        
        // Mostrar primer individuo de la poblacion
        cout << "\n=== PRIMER INDIVIDUO DE LA POBLACIoN ===" << endl;
        population[0].print(false);

        cout << "\n   Sistema de representacion poliploide implementado exitosamente" << endl;
        // Evaluar todas las politicas del primer individuo
        evaluateAllPolicies(ind1, scenario,"1");
        evaluateAllPolicies(ind2, scenario,"2");
        evaluateAllPolicies(population[0], scenario,"Poblacion");
        printHeader("acidos bechelours Jhoseph Flowereanous tiene mucha tarea debido a que tiene que mudarse de depa" ,20);
    } catch (const exception& e) {
        cerr << "EXCEPCIoN: " << e.what() << endl;
        return 1;
    }
    return 0;
}