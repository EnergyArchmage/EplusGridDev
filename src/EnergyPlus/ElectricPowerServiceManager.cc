
// C++ Headers
#include <vector>

// ObjexxFCL Headers
#include <ObjexxFCL/Array1.hh>

// EnergyPlus Headers
#include <ElectricPowerServiceManager.hh>
#include <PlantLocation.hh>

namespace EnergyPlus {

namespace ElectricPowerService {



	void
	ElectricPowerServiceManager::manageElectricLoadCenters(
		bool const FirstHVACIteration,
		bool & SimElecCircuits, // simulation convergence flag
		bool const UpdateMetersOnly // if true then don't resimulate generators, just update meters.
	)
	{

		if ( this->getInput ) {
			this-> getPowerManagerInput();
			this->getInput = false;
		}


	}
	void
	ElectricPowerServiceManager::getPowerManagerInput()
	{

	

	}


	void
	ElectricPowerServiceManager::updateWholeBuildingRecords()
	{
	
	}

	ElectPowerLoadCenter
	ElectPowerLoadCenter::electricLoadCenterFactory(
		std::string const objectName
	)
	{
	
	
	}

	void
	ElectPowerLoadCenter::calcLoadCenterThermalLoad(
		bool const firstHVACIteration, // unused1208
		int const loadCenterNum, // Load Center number counter
		Real64 & thermalLoad // heat rate called for from cogenerator(watts)
	)
	{
	
	}

	void
	ElectPowerLoadCenter::verifyCustomMetersElecPowerMgr()
	{
	
	}

	GeneratorController
	GeneratorController::generatorControllerFactory(
		std::string const objectName 
	)
	{
		
	}

	void
	GeneratorController::simGeneratorGetPowerOutput(
		int const loadCenterNum, // Load Center number counter
		int const genNum, // Generator number counter
		bool const firstHVACIteration, // Unused 2010 JANUARY
		Real64 & electricPowerOutput, // Actual generator electric power output
		Real64 & thermalPowerOutput // Actual generator thermal power output
	)
	{
	
	}

	DCtoACInverter
	DCtoACInverter::dCtoACInverterFactory(
		std::string const objectName
	)
	{
	
	}

	void
	DCtoACInverter::manageInverter(
		int const LoadCenterNum 
	)
	{
	
	}

	void
	DCtoACInverter::figureInverterZoneGains(
	)
	{
	
	}

	ElectricStorage
	ElectricStorage::electricStorageFactory(
		std::string const objectName
		// need object type
	)
	{
	
	}


	void
	ElectricStorage::manageElectCenterStorageInteractions(
		int const LoadCenterNum, // load center number, index for structure
		Real64 & StorageDrawnPower, // Electric Power Draw Rate from storage units
		Real64 & StorageStoredPower // Electric Power Store Rate from storage units
	)
	{
	
	}


	void
	ElectricStorage::FigureElectricalStorageZoneGains()
	{
	
	}

	bool
	ElectricStorage::determineCurrentForBatteryDischarge(
		Real64& curI0,
		Real64& curT0,
		Real64& curVolt,
		Real64 const Pw,
		Real64 const q0,
		int const CurveNum,
		Real64 const k,
		Real64 const c,
		Real64 const qmax,
		Real64 const E0c,
		Real64 const InternalR
	)
	{
	
	}

	void
	ElectricStorage::rainflow(
		int const numbin, // numbin = constant value
		Real64 const input, // input = input value from other object (battery model)
		std::vector < Real64 > B1, // stores values of points, calculated here - stored for next timestep
		std::vector < Real64 > X, // stores values of two data point difference, calculated here - stored for next timestep
		int & count, // calculated here - stored for next timestep in main loop
		std::vector < Real64 > Nmb, // calculated here - stored for next timestep in main loop
		std::vector < Real64 > OneNmb, // calculated here - stored for next timestep in main loop
		int const dim // end dimension of array
	)
	{
	
	}

	void
	ElectricStorage::shift(
		std::vector < Real64 > A,
		int const m,
		int const n,
		std::vector < Real64 > B,
		int const dim // end dimension of arrays
	)
	{
	
	}

	ElectricTransformer
	ElectricTransformer::electricTransformerFactory(
		std::string const objectName
	)
	{
	
	}

	void
	ElectricTransformer::manageTransformers()
	{
	
	}


} // ElectricPowerService namespace

} // EnergyPlus namespace
