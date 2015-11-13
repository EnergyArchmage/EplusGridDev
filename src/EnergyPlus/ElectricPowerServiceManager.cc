
// C++ Headers
#include <vector>

// ObjexxFCL Headers
#include <ObjexxFCL/Array1.hh>

// EnergyPlus Headers
#include <ElectricPowerServiceManager.hh>
#include <PlantLocation.hh>
#include <OutputProcessor.hh>
#include <DataGlobals.hh>
#include <UtilityRoutines.hh>
#include <InputProcessor.hh>
#include <DataIPShortCuts.hh>
#include <ScheduleManager.hh>

namespace EnergyPlus {

namespace ElectricPowerService {



	void
	ElectricPowerServiceManager::manageElectricLoadCenters(
		bool const FirstHVACIteration,
		bool & SimElecCircuits, // simulation convergence flag
		bool const UpdateMetersOnly // if true then don't resimulate generators, just update meters.
	)
	{

		if ( this->getInputFlag ) {
			this->getPowerManagerInput();
			this->getInputFlag = false;
		}

		if ( DataGlobals::MetersHaveBeenInitialized && this-> setupMeterIndexFlag ) {
			this->setupMeterIndices();
			this->setupMeterIndexFlag = false;
		}



	}
	void
	ElectricPowerServiceManager::getPowerManagerInput()
	{

	this->numLoadCenters = InputProcessor::GetNumObjectsFound( "ElectricLoadCenter:Distribution" );

	if ( this->numLoadCenters > 0 ){
		for ( auto iLoadCenterNum = 1; iLoadCenterNum <= this->numLoadCenters; ++iLoadCenterNum ){

			this->elecLoadCenterObjs.emplace_back( iLoadCenterNum );
		}
	
	}


	}

	void
	ElectricPowerServiceManager::setupMeterIndices()
	{
		elecFacilityIndex = EnergyPlus::GetMeterIndex( "Electricity:Facility" );
		elecProducedCoGenIndex = EnergyPlus::GetMeterIndex( "Cogeneration:ElectricityProduced" );
		elecProducedPVIndex = EnergyPlus::GetMeterIndex( "Photovoltaic:ElectricityProduced" );
		elecProducedWTIndex = EnergyPlus::GetMeterIndex( "WindTurbine:ElectricityProduced" );
		elecProducedStorageIndex = EnergyPlus::GetMeterIndex( "ElectricStorage:ElectricityProduced" );
	}

	void
	ElectricPowerServiceManager::updateWholeBuildingRecords()
	{
	
	}

	ElectPowerLoadCenter::ElectPowerLoadCenter(
		int const objectNum
	)
	{
		std::string const routineName = "ElectPowerLoadCenter constructor";
		int NumAlphas; // Number of elements in the alpha array
		int NumNums; // Number of elements in the numeric array
		int IOStat; // IO Status when calling get input subroutine
		bool errorsFound;

		DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Distribution";
		errorsFound = false;
		InputProcessor::GetObjectItem( DataIPShortCuts::cCurrentModuleObject, objectNum, DataIPShortCuts::cAlphaArgs, NumAlphas, DataIPShortCuts::rNumericArgs, NumNums, IOStat, DataIPShortCuts::lNumericFieldBlanks, DataIPShortCuts::lAlphaFieldBlanks, DataIPShortCuts::cAlphaFieldNames, DataIPShortCuts::cNumericFieldNames  );

		this->name          = DataIPShortCuts::cAlphaArgs( 1 );
		// how to verify names are unique across objects? add to GlobalNames?

		this->generatorList = DataIPShortCuts::cAlphaArgs( 2 );

		//Load the Generator Control Operation Scheme
		if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "Baseload" ) ) {
			this->genOperationScheme = genOpSchemeBaseLoad;
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "DemandLimit" ) ) {
			this->genOperationScheme = genOpSchemeDemandLimit
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "TrackElectrical" ) ) {
			this->genOperationScheme = genOpSchemeTrackElectrical;
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "TrackSchedule" ) ) {
			this->genOperationScheme = genOpSchemeTrackSchedule;
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "TrackMeter" ) ) {
			this->genOperationScheme =  genOpSchemeTrackMeter;
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "FollowThermal" ) ) {
			this->genOperationScheme = genOpSchemeThermalFollow;
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "FollowThermalLimitElectrical" ) ) {
			this->genOperationScheme =  genOpSchemeThermalFollowLimitElectrical;
		} else {
			ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
			ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 3 ) + " = " + DataIPShortCuts::cAlphaArgs( 3 ) );
			errorsFound = true;
		}

		this->demandLimit = DataIPShortCuts::rNumericArgs( 1 );

		this->trackSchedPtr = ScheduleManager::GetScheduleIndex( DataIPShortCuts::cAlphaArgs( 4 ) );
		if ( ( this->trackSchedPtr == 0 ) && ( this->genOperationScheme == genOpSchemeTrackSchedule ) ) {
			if ( ! DataIPShortCuts::lAlphaFieldBlanks( 4 ) ) {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
				ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + " = " + DataIPShortCuts::cAlphaArgs( 4 ) );
			} else {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
				ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + " = blank field." );
			}
			ShowContinueError( "Schedule not found; Must be entered and valid when Generator Operation Scheme=TrackSchedule" );
			errorsFound = true;
		
		}

		this->demandMeterName = InputProcessor::MakeUPPERCase( DataIPShortCuts::cAlphaArgs( 5 ) );
			// meters may not be "loaded" yet, defered check to later subroutine

		if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "AlternatingCurrent" ) ) {
			this->bussType = aCBuss;
			DataIPShortCuts::cAlphaArgs( 6 ) = "AlternatingCurrent";
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "DirectCurrentWithInverter" ) ) {
			this->bussType  = dCBussInverter;
			this->inverterPresent = true;
			DataIPShortCuts::cAlphaArgs( 6 ) = "DirectCurrentWithInverter";
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "AlternatingCurrentWithStorage" ) ) {
			this->bussType  = aCBussStorage;
			this->storagePresent = true;
			DataIPShortCuts::cAlphaArgs( 6 ) = "AlternatingCurrentWithStorage";
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "DirectCurrentWithInverterDCStorage" ) ) {
			this->bussType  = dCBussInverterDCStorage;
			this->inverterPresent = true;
			this->storagePresent = true;
			DataIPShortCuts::cAlphaArgs( 6 ) = "DirectCurrentWithInverterDCStorage";
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "DirectCurrentWithInverterACStorage" ) ) {
			this->bussType  = dCBussInverterACStorage;
			this->inverterPresent = true;
			this->storagePresent = true;
			DataIPShortCuts::cAlphaArgs( 6 ) = "DirectCurrentWithInverterACStorage";
		} else if ( DataIPShortCuts::cAlphaArgs( 6 ).empty() ) {
			this->bussType  = aCBuss;
			DataIPShortCuts::cAlphaArgs( 6 ) = "AlternatingCurrent (field was blank)";
		} else {
			ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
			ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 6 ) + " = " + DataIPShortCuts::cAlphaArgs( 6 ) );
			errorsFound = true;
		}

		if ( this->inverterPresent ) {

			this->inverterOjb = std::make_unique< DCtoACInverter >( DataIPShortCuts::cAlphaFieldNames( 7 )  );

		}



	
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

	GeneratorController::GeneratorController(
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

	DCtoACInverter::DCtoACInverter(
		std::string const objectName
	)
	{
		// if/when add object class name to input object this can be simplified. for now search all possible types 
		bool foundInverter = false;
		int testInvertIndex = 0;

		testInvertIndex = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Inverter:LookUpTable",  objectName );
		if ( testInvertIndex > 0) {
			foundInverter = true;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Inverter:LookUpTable";
			this->modelType = cECLookUpTableModel;
		}
		testInvertIndex = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Inverter:FunctionOfPower",  objectName );
		if ( testInvertIndex > 0) {
			foundInverter = true;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Inverter:FunctionOfPower";
			this->modelType = curveFuncOfPower;
		}
		testInvertIndex = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Inverter:Simple",  objectName );
		if ( testInvertIndex > 0) {
			foundInverter = true;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Inverter:Simple";
			this->modelType = simpleConstantEff;
		}

		if ( foundInverter ){

			switch ( this->modelType )
			{
			case cECLookUpTableModel: {
			
				break;
			}
			case curveFuncOfPower: {
			
				break;
			}
			case simpleConstantEff: {
			
				break;
			}

			}
		
		
		
		
		
		}

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


	ElectricStorage::ElectricStorage(
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


	ElectricTransformer::ElectricTransformer(
		std::string const objectName
	)
	{
	
	}

	void
	ElectricTransformer::manageTransformers()
	{
	
	}

	void
	ElectricTransformer::setupMeterIndices()
	{
		if (this->usageMode == powerInFromGrid ) {
			for ( auto meterNum = 1; meterNum <= this->wiredMeterNames.size; ++meterNum ) {

				this->wiredMeterPtrs[ meterNum ] = GetMeterIndex( this->wiredMeterNames[ meterNum ] );

				//Check whether the meter is an electricity meter
				//Index function is used here because some resource types are not Electricity but strings containing
				// Electricity such as ElectricityPurchased and ElectricityProduced.
				//It is not proper to have this check in GetInput routine because the meter index may have not been defined
				if ( ! has( GetMeterResourceType( this->wiredMeterPtrs[ meterNum ] ), "Electricity" ) ) {
					EnergyPlus::ShowFatalError( "Non-electricity meter used for " + this->name );
				}
			}
		}
	}

} // ElectricPowerService namespace

} // EnergyPlus namespace
