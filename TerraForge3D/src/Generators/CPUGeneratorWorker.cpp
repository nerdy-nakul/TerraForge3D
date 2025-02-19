#include "Generators/CPUGeneratorWorker.h"
#include "Generators/MeshGeneratorManager.h"

#include "Utils/Utils.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"

#include "Generators/CPUNoiseLayersGenerator.h"
#include "Generators/GPUNoiseLayerGenerator.h"
#include "Base/NodeEditor/NodeEditor.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include "Base/UIFontManager.h"

#include "Profiler.h"
#include "Platform.h"


CPUGeneratorWorker::CPUGeneratorWorker(ApplicationState* appState, int id)
{
	this->appState = appState;
	this->id = id;
	this->current_state = CPUGeneratorWorkerState_Dead;
}

CPUGeneratorWorker::~CPUGeneratorWorker()
{
	Quit();
}

void CPUGeneratorWorker::AddJob(std::vector<CPUNoiseLayersGenerator*>& cpuNoiseLayers, std::vector<CPUNodeEditor*> cpuNodeEditors, int start, int size)
{
	jobs.push(std::make_pair(cpuNoiseLayers, cpuNodeEditors));
	jobs_sizes.push(std::make_pair(start, size));
}

void CPUGeneratorWorker::Start()
{
	if (this->current_state != CPUGeneratorWorkerState_Dead) return;
	this->worker_th = std::thread(std::bind(&CPUGeneratorWorker::Worker, this));
	this->worker_th.detach();
}

void CPUGeneratorWorker::Quit()
{
	this->request_quit = true;
	if (worker_th.joinable()) worker_th.join();
}

void CPUGeneratorWorker::WaitForFinish()
{
	while (this->jobs.size() > 0 || this->current_state == CPUGeneratorWorkerState_Working);
}

void CPUGeneratorWorker::Worker()
{
	std::cout << "Worker [ID:" << id << "] started\n";

	while (!this->request_quit)
	{
		this->current_state = CPUGeneratorWorkerState_Waiting;
		while (!(jobs.size() > 0 && jobs.size() == jobs_sizes.size())) { using namespace std::chrono_literals; std::this_thread::sleep_for((id < this->appState->globals.cpuWorkerThreadsActive) ? 10ms : 1000ms); }
		this->current_state = CPUGeneratorWorkerState_Working;
		this->progress = 0.0f;

		auto job_layers = jobs.front(); jobs.pop();
		auto job_sz = jobs_sizes.front(); jobs_sizes.pop();

		float stepD = appState->mainMap.tileSize / appState->mainMap.tileResolution;

		START_PROFILER();

		for (int i = job_sz.first; i < job_sz.first + job_sz.second; i++)
		{
			for (int j = 0; j < appState->mainMap.tileResolution; j++)
			{
				float elev = 0.0f;
				appState->mainMap.currentTileDataLayers[0]->GetPixelI(j, i, &elev);
				NodeInputParam inp{};
				inp.x = appState->mainMap.tileOffsetX + j * stepD;
				inp.y = elev;
				inp.z = appState->mainMap.tileOffsetY + i * stepD;
				inp.minX = 0.0f; inp.minY = 0.0f; inp.minZ = 0.0f;
				inp.maxX = 1.0f; inp.maxY = 1.0f; inp.maxZ = 1.0f;
				inp.texX = inp.x;
				inp.texY = inp.z;
				inp.userData3 = appState->mainMap.currentTileDataLayers[1];
				float el0 = 0.0f, el1 = 0.0f;


				auto& cpuNoiseLayers = job_layers.first;
				for (int j = 0; j < cpuNoiseLayers.size(); j++)
				{
					if (cpuNoiseLayers[j]->enabled)
					{
						inp.y = elev;
						elev = cpuNoiseLayers[j]->EvaluateAt(inp.x, inp.y, inp.z);
					}
				}
				el0 = elev;

				auto& cpuNodeEditors = job_layers.second;
				for (int j = 0; j < cpuNodeEditors.size(); j++)
				{
					if (cpuNodeEditors[j]->enabled)
					{
						inp.y = elev;
						elev = cpuNodeEditors[j]->EvaluateAt(inp);
					}
				}
				el1 = elev - el0;

				appState->mainMap.currentTileDataLayers[0]->SetPixelI(j, i, elev, el0, el1, 0.0f);
			}
			this->progress = float(i - job_sz.first) / job_sz.second;
		}

		END_PROFILER(jobTime);

		this->progress = 1.0f;

	}

	std::cout << "Worker [ID:" << id << "] terminated\n";
	this->current_state = CPUGeneratorWorkerState_Dead;
}
