
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
#include <CurveManager.hh>
#include <DataHeatBalance.hh>
#include <HeatBalanceInternalHeatGains.hh>
#include <EMSManager.hh>
#include <General.hh>

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

			this->elecLoadCenterObjs.emplace_back( std::make_unique <ElectPowerLoadCenter > ( iLoadCenterNum) );
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
		int objectNum
	)
	{
		// initialize 
		this->name="";
		this->generatorListName="";
		this->genOperationScheme = genOpSchemeNotYetSet ;
		this->demandMeterPtr = 0;
		this->numGenerators = 0;
		this->demandLimit = 0.0;
		this->trackSchedPtr = 0;
		this->bussType = bussNotYetSet;
		this->inverterPresent = false;
		this->inverterModelNum = 0;
		this->dCElectricityProd = 0.0;
		this->dCElectProdRate = 0.0;
		this->dCpowerConditionLosses = 0.0;
		this->storagePresent = false;
		this->storageModelNum = 0;
		this->transformerPresent = false;
		this->transformerModelNum = 0;
		this->electricityProd = 0.0;
		this->electProdRate = 0.0;
		this->thermalProd = 0.0;
		this->thermalProdRate = 0.0;
		this->totalPowerRequest = 0.0;
		this->totalThermalPowerRequest = 0.0;
		this->electDemand = 0.0;

		std::string const routineName = "ElectPowerLoadCenter constructor ";
		int numAlphas; // Number of elements in the alpha array
		int numNums; // Number of elements in the numeric array
		int IOStat; // IO Status when calling get input subroutine
		bool errorsFound;

		DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Distribution";
		errorsFound = false;
		InputProcessor::GetObjectItem( DataIPShortCuts::cCurrentModuleObject, objectNum, DataIPShortCuts::cAlphaArgs, numAlphas, DataIPShortCuts::rNumericArgs, numNums, IOStat, DataIPShortCuts::lNumericFieldBlanks, DataIPShortCuts::lAlphaFieldBlanks, DataIPShortCuts::cAlphaFieldNames, DataIPShortCuts::cNumericFieldNames  );

		this->name          = DataIPShortCuts::cAlphaArgs( 1 );
		// how to verify names are unique across objects? add to GlobalNames?

		this->generatorListName = DataIPShortCuts::cAlphaArgs( 2 );

		//Load the Generator Control Operation Scheme
		if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "Baseload" ) ) {
			this->genOperationScheme = genOpSchemeBaseLoad;
		} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 3 ), "DemandLimit" ) ) {
			this->genOperationScheme = genOpSchemeDemandLimit;
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
			if ( !  DataIPShortCuts::lAlphaFieldBlanks( 7 ) ) {
				this->inverterName = DataIPShortCuts::cAlphaFieldNames( 7 );
			} else {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
				ShowContinueError( DataIPShortCuts::cAlphaFieldNames( 7 ) + " is blank, but buss type requires inverter.");
				errorsFound = true;
			}

		}

		if ( this->storagePresent ) {
			if ( ! DataIPShortCuts::lAlphaFieldBlanks( 8 ) ) {
				this->storageName = DataIPShortCuts::cAlphaFieldNames( 8 );
			} else {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
				ShowContinueError( DataIPShortCuts::cAlphaFieldNames( 8 ) + " is blank, but buss type requires storage.");
				errorsFound = true;
			}
		}

		if ( ! DataIPShortCuts::lAlphaFieldBlanks( 9 ) ) {
			// process transformer
			this->transformerName = DataIPShortCuts::cAlphaFieldNames( 9 );
			this->transformerPresent =  true;
		}

		// now that we are done with processing get input for ElectricLoadCenter:Distribution we can call child input objects without IP shortcut problems

		DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Generators";
		int genListObjectNum = InputProcessor::GetObjectItemNum( DataIPShortCuts::cCurrentModuleObject, this->generatorListName );
		if ( genListObjectNum > 0 ){
			InputProcessor::GetObjectItem( DataIPShortCuts::cCurrentModuleObject, objectNum, DataIPShortCuts::cAlphaArgs, numAlphas, DataIPShortCuts::rNumericArgs, numNums, IOStat, DataIPShortCuts::lNumericFieldBlanks, DataIPShortCuts::lAlphaFieldBlanks, DataIPShortCuts::cAlphaFieldNames, DataIPShortCuts::cNumericFieldNames  );

			//Calculate the number of generators in list
			this->numGenerators = numNums / 2; // note IDD needs Min Fields = 6  
			if ( mod( ( numAlphas - 1 + numNums ), 5 ) != 0 ) ++this->numGenerators;
			int alphaCount = 2;
			for ( auto genCount = 1; genCount <= this->numGenerators; ++genCount) {
				// call constructor in place
				this->elecGenCntrlObj.emplace_back( std::make_unique < GeneratorController >( DataIPShortCuts::cAlphaArgs( alphaCount ), DataIPShortCuts::cAlphaArgs( alphaCount + 1 ), DataIPShortCuts::rNumericArgs( 2 * genCount - 1 ), DataIPShortCuts::cAlphaArgs( alphaCount + 2 ), DataIPShortCuts::rNumericArgs( 2 * genCount) ) );
				++alphaCount;
				++alphaCount;
				++alphaCount;
			}

		}


		if ( ! errorsFound && this->inverterPresent ) {
			// call inverter constructor
			std::string thisInverterName = this->inverterName;
			//this->inverterObj = std::unique_ptr< DCtoACInverter >( new DCtoACInverter( thisInverterName ) );

			this->inverterObj = std::make_unique< DCtoACInverter >  ( thisInverterName ) ;
			//this->inverterObj = new DCtoACInverter ( thisInverterName ) ;
		}

		if ( ! errorsFound && this->storagePresent ) {
			// call storage constructor 
			this->storageObj =  std::make_unique< ElectricStorage >(  this->storageName  );
		}

		if ( ! errorsFound && this->transformerPresent ) {
			//call transformer constructor 
			this->transformerObj = std::make_unique< ElectricTransformer >( this->transformerName  );
		
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
		std::string objectName,
		std::string objectType,
		Real64 ratedElecPowerOutput,
		std::string availSchedName,
		Real64 thermalToElectRatio
	)
	{
		std::string const routineName = "GeneratorController constructor ";
		bool errorsFound = false;

		this->name                   = objectName;
		this->typeOfName             = objectType;
		if ( InputProcessor::SameString( objectType, "Generator:InternalCombustionEngine" ) ) {
			this->generatorType = generatorICEngine;
		} else if ( InputProcessor::SameString( objectType, "Generator:CombustionTurbine" ) ) {
			this->generatorType = generatorCombTurbine;
		} else if ( InputProcessor::SameString( objectType, "Generator:MicroTurbine" ) ) {
			this->generatorType = generatorMicroturbine;
		} else if ( InputProcessor::SameString( objectType, "Generator:Photovoltaic" ) ) {
			this->generatorType = generatorPV;
		} else if ( InputProcessor::SameString( objectType, "Generator:FuelCell" ) ) {
			this->generatorType = generatorFuelCell;
		} else if ( InputProcessor::SameString( objectType, "Generator:MicroCHP" ) ) {
			this->generatorType = generatorMicroCHP;
		} else if ( InputProcessor::SameString( objectType, "Generator:WindTurbine" ) ) {
			this->generatorType = generatorWindTurbine;
		} else {
			ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + " invalid entry." );
			ShowContinueError( "Invalid " + objectType + " associated with generator = " + objectName ) ;
			errorsFound = true;
		}

		this->availSched             = availSchedName;
		if ( availSched.empty() ) {
			this->availSchedPtr = DataGlobals::ScheduleAlwaysOn;
		} else {
			this->availSchedPtr =  ScheduleManager::GetScheduleIndex( availSchedName );
			if ( this->availSchedPtr <= 0 ) {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + ", invalid entry." );
				ShowContinueError( "Invalid availability schedule = " + availSchedName );
				ShowContinueError( "Schedule was not found " );
				errorsFound = true;
			}
		}

		this->maxPowerOut            = ratedElecPowerOutput,
		this->nominalThermElectRatio = thermalToElectRatio;

		SetupOutputVariable( "Generator Requested Electric Power [W]", this->powerRequestThisTimestep, "System", "Average", objectName );
		if ( DataGlobals::AnyEnergyManagementSystemInModel ) {
			SetupEMSInternalVariable( "Generator Nominal Maximum Power", objectName, "[W]", this->maxPowerOut );
			SetupEMSInternalVariable( "Generator Nominal Thermal To Electric Ratio", objectName, "[ratio]", this->nominalThermElectRatio );

			SetupEMSActuator( "On-Site Generator Control", objectName, "Requested Power", "[W]", this->eMSRequestOn, this->eMSPowerRequest );

		}

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
		std::string objectName
	)
	{
		//initialize
		this->modelType = notYetSet;
		this->availSchedPtr = 0;
		this->heatLossesDestination = heatLossNotDetermined;
		this->zoneNum = 0;
		this->zoneRadFract = 0.0;
		this->nightTareLossPower = 0.0;
		this->nominalVoltage = 0.0;
		this->nomVoltEfficiencyARR.resize( 6, 0.0 );
		this->curveNum = 0;
		this->ratedPower = 0.0;
		this->minPower = 0.0;
		this->maxPower = 0.0;
		this->minEfficiency = 0.0;
		this->maxEfficiency = 0.0;
		this->standbyPower = 0.0;
		this->efficiency = 0.0;
		this->dCPowerIn = 0.0;
		this->aCPowerOut = 0.0;
		this->dCEnergyIn = 0.0;
		this->aCEnergyOut = 0.0;
		this->thermLossRate = 0.0;
		this->thermLossEnergy = 0.0;
		this->qdotConvZone = 0.0;
		this->qdotRadZone = 0.0;
		this->ancillACuseRate = 0.0;
		this->ancillACuseEnergy = 0.0;

		std::string const routineName = "DCtoACInverter constructor ";
		int NumAlphas; // Number of elements in the alpha array
		int NumNums; // Number of elements in the numeric array
		int IOStat; // IO Status when calling get input subroutine
		bool errorsFound = false;
		// if/when add object class name to input object this can be simplified. for now search all possible types 
		bool foundInverter = false;
		int testInvertIndex = 0;
		int invertIDFObjectNum = 0;

		testInvertIndex = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Inverter:LookUpTable",  objectName );
		if ( testInvertIndex > 0) {
			foundInverter = true;
			invertIDFObjectNum = testInvertIndex;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Inverter:LookUpTable";
			this->modelType = cECLookUpTableModel;
		}
		testInvertIndex = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Inverter:FunctionOfPower",  objectName );
		if ( testInvertIndex > 0) {
			foundInverter = true;
			invertIDFObjectNum = testInvertIndex;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Inverter:FunctionOfPower";
			this->modelType = curveFuncOfPower;
		}
		testInvertIndex = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Inverter:Simple",  objectName );
		if ( testInvertIndex > 0) {
			foundInverter = true;
			invertIDFObjectNum = testInvertIndex;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Inverter:Simple";
			this->modelType = simpleConstantEff;
		}

		if ( foundInverter ){

			InputProcessor::GetObjectItem( DataIPShortCuts::cCurrentModuleObject, invertIDFObjectNum, DataIPShortCuts::cAlphaArgs, NumAlphas, DataIPShortCuts::rNumericArgs, NumNums, IOStat, DataIPShortCuts::lNumericFieldBlanks, DataIPShortCuts::lAlphaFieldBlanks, DataIPShortCuts::cAlphaFieldNames, DataIPShortCuts::cNumericFieldNames  );

			this->name          = DataIPShortCuts::cAlphaArgs( 1 );
			// how to verify names are unique across objects? add to GlobalNames?

			if ( DataIPShortCuts::lAlphaFieldBlanks( 2 ) ) {
				this->availSchedPtr = DataGlobals::ScheduleAlwaysOn;
			} else {
				this->availSchedPtr = ScheduleManager::GetScheduleIndex( DataIPShortCuts::cAlphaArgs( 2 ) );
				if ( this->availSchedPtr == 0 ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 2 ) + " = " + DataIPShortCuts::cAlphaArgs( 2 ) );
					errorsFound = true;
				}
			}

			this->zoneNum = InputProcessor::FindItemInList( DataIPShortCuts::cAlphaArgs( 3 ), DataHeatBalance::Zone );
			if ( this->zoneNum > 0 ) this->heatLossesDestination = zoneGains;
			if ( this->zoneNum == 0 ) {
				if ( DataIPShortCuts::lAlphaFieldBlanks( 3 ) ) {
					this->heatLossesDestination = lostToOutside;
				} else {
					this->heatLossesDestination = lostToOutside;
					ShowWarningError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 3 ) + " = " + DataIPShortCuts::cAlphaArgs( 3 ) );
					ShowContinueError( "Zone name not found. Inverter heat losses will not be added to a zone" );
					// continue with simulation but inverter losses not sent to a zone.
				}
			}
			this->zoneRadFract = DataIPShortCuts::rNumericArgs( 1 );

			// now the input objects differ depending on class type
			switch ( this->modelType )
			{
			case cECLookUpTableModel: {
				this->ratedPower                = DataIPShortCuts::rNumericArgs( 2 );
				this->standbyPower              = DataIPShortCuts::rNumericArgs( 3 );
				this->nightTareLossPower        = DataIPShortCuts::rNumericArgs( 3 );
				this->nominalVoltage            = DataIPShortCuts::rNumericArgs( 4 );

				this->nomVoltEfficiencyARR[ 1 ] = DataIPShortCuts::rNumericArgs( 5 );
				this->nomVoltEfficiencyARR[ 2 ] = DataIPShortCuts::rNumericArgs( 6 );
				this->nomVoltEfficiencyARR[ 3 ] = DataIPShortCuts::rNumericArgs( 7 );
				this->nomVoltEfficiencyARR[ 4 ] = DataIPShortCuts::rNumericArgs( 8 );
				this->nomVoltEfficiencyARR[ 5 ] = DataIPShortCuts::rNumericArgs( 9 );
				this->nomVoltEfficiencyARR[ 6 ] = DataIPShortCuts::rNumericArgs( 10 );
				break;
			}
			case curveFuncOfPower: {
				this->curveNum = CurveManager::GetCurveIndex( DataIPShortCuts::cAlphaArgs( 4 ) );
				if ( this->curveNum == 0 ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + " = " + DataIPShortCuts::cAlphaArgs( 4 ) );
					ShowContinueError( "Curve was not found" );
					errorsFound = true;
				}

				this->ratedPower    = DataIPShortCuts::rNumericArgs( 2 );
				this->minEfficiency = DataIPShortCuts::rNumericArgs( 3 );
				this->maxEfficiency = DataIPShortCuts::rNumericArgs( 4 );
				this->minPower      = DataIPShortCuts::rNumericArgs( 5 );
				this->maxPower      = DataIPShortCuts::rNumericArgs( 6 );
				this->standbyPower  = DataIPShortCuts::rNumericArgs( 7 );
				break;
			}
			case simpleConstantEff: {
				this->efficiency = DataIPShortCuts::rNumericArgs( 2 );
				break;
			}

			} // end switch modelType
		
			SetupOutputVariable( "Inverter DC to AC Efficiency []", this->efficiency, "System", "Average", this->name );
			SetupOutputVariable( "Inverter DC Input Electric Power [W]", this->dCPowerIn, "System", "Average", this->name );
			SetupOutputVariable( "Inverter DC Input Electric Energy [J]", this->dCEnergyIn, "System", "Sum", this->name );
			SetupOutputVariable( "Inverter AC Output Electric Power [W]", this->aCPowerOut, "System", "Average", this->name );
			SetupOutputVariable( "Inverter AC Output Electric Energy [J]", this->aCEnergyOut, "System", "Sum", this->name, _, "ElectricityProduced", "Photovoltaics", _, "Plant" ); // right now PV is the only DC source
			SetupOutputVariable( "Inverter Thermal Loss Rate [W]", this->thermLossRate, "System", "Average", this->name );
			SetupOutputVariable( "Inverter Thermal Loss Energy [J]", this->thermLossEnergy, "System", "Sum", this->name );
			SetupOutputVariable( "Inverter Ancillary AC Electric Power [W]", this->ancillACuseRate, "System", "Average", this->name );
			SetupOutputVariable( "Inverter Ancillary AC Electric Energy [J]", this->ancillACuseEnergy, "System", "Sum", this->name, _, "Electricity", "Cogeneration", _, "Plant" ); // called cogeneration for end use table
			if ( this->zoneNum > 0 ) {
				switch (this->modelType )
				{
				case simpleConstantEff: {
					SetupZoneInternalGain( this->zoneNum, "ElectricLoadCenter:Inverter:Simple", this->name, DataHeatBalance::IntGainTypeOf_ElectricLoadCenterInverterSimple, this->qdotConvZone, _, this->qdotRadZone );
					break;
				}
				case curveFuncOfPower: {
					SetupZoneInternalGain( this->zoneNum, "ElectricLoadCenter:Inverter:FunctionOfPower", this->name, DataHeatBalance::IntGainTypeOf_ElectricLoadCenterInverterFunctionOfPower, this->qdotConvZone, _, this->qdotRadZone );
					break;
				}
				case cECLookUpTableModel: {
					SetupZoneInternalGain( this->zoneNum, "ElectricLoadCenter:Inverter:LookUpTable", this->name, DataHeatBalance::IntGainTypeOf_ElectricLoadCenterInverterLookUpTable, this->qdotConvZone, _, this->qdotRadZone );
					break;
				}

				} // end switch modelType

			}

		} else {
			ShowSevereError( routineName + " did not find inverter name = " + objectName);
			errorsFound = true;
		}

		if ( errorsFound ) {
			ShowFatalError( routineName + "Preceding errors terminate program." );
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
		std::string objectName
	)
	{
		//initialize
		this->storageModelMode = storageTypeNotSet;
		this->availSchedPtr = 0;
		this->heatLossesDestination = heatLossNotDetermined;
		this->zoneNum = 0;
		this->zoneRadFract = 0.0;
		this->startingEnergyStored = 0.0;
		this->energeticEfficCharge = 0.0;
		this->energeticEfficDischarge = 0.0;
		this->maxPowerDraw = 0.0;
		this->maxPowerStore = 0.0;
		this->maxEnergyCapacity = 0.0;
		this->parallelNum = 0;
		this->seriesNum = 0;
		this->chargeCurveNum = 0;
		this->dischargeCurveNum = 0;
		this->cycleBinNum = 0;
		this->startingSOC = 0.0;
		this->maxAhCapacity = 0.0;
		this->availableFrac = 0.0;
		this->chargeConversionRate = 0.0;
		this->chargedOCV = 0.0;
		this->dischargedOCV = 0.0;
		this->internalR = 0.0;
		this->maxDischargeI = 0.0;
		this->cutoffV = 0.0;
		this->maxChargeRate = 0.0;
		this->lifeCalculation = degredationNotSet;
		this->lifeCurveNum = 0;
		this->thisTimeStepStateOfCharge = 0.0;
		this->lastTimeStepStateOfCharge = 0.0;
		this->pelNeedFromStorage = 0.0;
		this->pelFromStorage = 0.0;
		this->eMSOverridePelFromStorage = false;
		this->eMSValuePelFromStorage = 0.0;
		this->pelIntoStorage = 0.0;
		this->eMSOverridePelIntoStorage = false;
		this->eMSValuePelIntoStorage = 0.0;
		this->qdotConvZone = 0.0;
		this->qdotRadZone = 0.0;
		this->timeElapsed = 0.0;
		this->thisTimeStepAvailable = 0.0;
		this->thisTimeStepBound = 0.0;
		this->lastTimeStepAvailable = 0.0;
		this->lastTimeStepBound = 0.0;
		this->lastTwoTimeStepAvailable = 0.0;
		this->lastTwoTimeStepBound = 0.0;
		this->count0 = 0;
		this->electEnergyinStorage = 0.0;
		this->storedPower = 0.0;
		this->storedEnergy = 0.0;
		this->decrementedEnergyStored = 0.0;
		this->drawnPower = 0.0;
		this->drawnEnergy = 0.0;
		this->thermLossRate = 0.0;
		this->thermLossEnergy = 0.0;
		this->storageMode = 0;
		this->absoluteSOC = 0.0;
		this->fractionSOC = 0.0;
		this->batteryCurrent = 0.0;
		this->batteryVoltage = 0.0;
		this->batteryDamage = 0.0;

		std::string const routineName = "ElectricStorage constructor ";
		int NumAlphas; // Number of elements in the alpha array
		int NumNums; // Number of elements in the numeric array
		int IOStat; // IO Status when calling get input subroutine
		bool errorsFound = false;
		// if/when add object class name to input object this can be simplified. for now search all possible types 
		bool foundStorage = false;
		int testStorageIndex = 0;
		int storageIDFObjectNum = 0;

		testStorageIndex = InputProcessor::GetObjectItemNum("ElectricLoadCenter:Storage:Simple", objectName );
		if ( testStorageIndex > 0 ) {
			foundStorage = true;
			storageIDFObjectNum = testStorageIndex;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Storage:Simple";
			this->storageModelMode = simpleBucketStorage;
		}

		testStorageIndex = InputProcessor::GetObjectItemNum("ElectricLoadCenter:Storage:Battery", objectName );
		if ( testStorageIndex > 0 ) {
			foundStorage = true;
			storageIDFObjectNum = testStorageIndex;
			DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Storage:Battery";
			this->storageModelMode = kiBaMBattery;
		}

		if ( foundStorage ) {
			InputProcessor::GetObjectItem( DataIPShortCuts::cCurrentModuleObject, storageIDFObjectNum, DataIPShortCuts::cAlphaArgs, NumAlphas, DataIPShortCuts::rNumericArgs, NumNums, IOStat, DataIPShortCuts::lNumericFieldBlanks, DataIPShortCuts::lAlphaFieldBlanks, DataIPShortCuts::cAlphaFieldNames, DataIPShortCuts::cNumericFieldNames  );

			this->name          = DataIPShortCuts::cAlphaArgs( 1 );
			// how to verify names are unique across objects? add to GlobalNames?

			if ( DataIPShortCuts::lAlphaFieldBlanks( 2 ) ) {
				this->availSchedPtr = DataGlobals::ScheduleAlwaysOn;
			} else {
				this->availSchedPtr = ScheduleManager::GetScheduleIndex( DataIPShortCuts::cAlphaArgs( 2 ) );
				if ( this->availSchedPtr == 0 ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 2 ) + " = " + DataIPShortCuts::cAlphaArgs( 2 ) );
					errorsFound = true;
				}
			}

			this->zoneNum = InputProcessor::FindItemInList( DataIPShortCuts::cAlphaArgs( 3 ), DataHeatBalance::Zone );
			if ( this->zoneNum > 0 ) this->heatLossesDestination = zoneGains;
			if ( this->zoneNum == 0 ) {
				if ( DataIPShortCuts::lAlphaFieldBlanks( 3 ) ) {
					this->heatLossesDestination = lostToOutside;
				} else {
					this->heatLossesDestination = lostToOutside;
					ShowWarningError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 3 ) + " = " + DataIPShortCuts::cAlphaArgs( 3 ) );
					ShowContinueError( "Zone name not found. Storage heat losses will not be added to a zone" );
					// continue with simulation but storage losses not sent to a zone.
				}
			}
			this->zoneRadFract = DataIPShortCuts::rNumericArgs( 1 );

			switch ( this->storageModelMode )
			{
			
			case simpleBucketStorage: {
				this->energeticEfficCharge    = DataIPShortCuts::rNumericArgs( 2 );
				this->energeticEfficDischarge = DataIPShortCuts::rNumericArgs( 3 );
				this->maxEnergyCapacity       = DataIPShortCuts::rNumericArgs( 4 );
				this->maxPowerDraw            = DataIPShortCuts::rNumericArgs( 5 );
				this->maxPowerStore           = DataIPShortCuts::rNumericArgs( 6 );
				this->startingEnergyStored    = DataIPShortCuts::rNumericArgs( 7 );
				SetupOutputVariable( "Electric Storage Charge State [J]", this->electEnergyinStorage, "System", "Average", this->name );
				break;
			}
			
			case kiBaMBattery: {
				this->chargeCurveNum = CurveManager::GetCurveIndex( DataIPShortCuts::cAlphaArgs( 4 ) ); //voltage calculation for charging
				if ( this->chargeCurveNum == 0 && ! DataIPShortCuts::lAlphaFieldBlanks( 4 ) ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + '=' + DataIPShortCuts::cAlphaArgs( 4 ) );
					errorsFound = true;
				} else if ( DataIPShortCuts::lAlphaFieldBlanks( 4 ) ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + " cannot be blank. But no entry found." );
					errorsFound = true;
				} else if ( ! InputProcessor::SameString( CurveManager::GetCurveType( this->chargeCurveNum ), "RectangularHyperbola2" ) ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + '=' + DataIPShortCuts::cAlphaArgs( 4 ) );
					ShowContinueError( "Curve Type must be RectangularHyperbola2 but was " + CurveManager::GetCurveType( this->chargeCurveNum ) );
					errorsFound = true;
				}
				this->dischargeCurveNum = CurveManager::GetCurveIndex( DataIPShortCuts::cAlphaArgs( 5 ) ); // voltage calculation for discharging
				if ( this->dischargeCurveNum == 0 && ! DataIPShortCuts::lAlphaFieldBlanks( 5 ) ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 5 ) + '=' + DataIPShortCuts::cAlphaArgs( 5 ) );
					errorsFound = true;
				} else if ( DataIPShortCuts::lAlphaFieldBlanks( 5 ) ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 5 ) + " cannot be blank. But no entry found." );
					errorsFound = true;
				} else if ( ! InputProcessor::SameString( CurveManager::GetCurveType( this->dischargeCurveNum ), "RectangularHyperbola2" ) ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 5 ) + '=' + DataIPShortCuts::cAlphaArgs( 5 ) );
					ShowContinueError( "Curve Type must be RectangularHyperbola2 but was " + CurveManager::GetCurveType( this->dischargeCurveNum ) );
					errorsFound = true;
				}

				if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "Yes" ) ) {
					this->lifeCalculation = batteryLifeCalculationYes;
				} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "No" ) ) {
					this->lifeCalculation = batteryLifeCalculationNo;
				} else {
					ShowWarningError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 6 ) + " = " + DataIPShortCuts::cAlphaArgs( 6 ) );
					ShowContinueError( "Yes or No should be selected. Default value No is used to continue simulation" );
					this->lifeCalculation = batteryLifeCalculationNo;
				}

				if ( this->lifeCalculation == batteryLifeCalculationYes ) {
					this->lifeCurveNum = CurveManager::GetCurveIndex( DataIPShortCuts::cAlphaArgs( 7 ) ); //Battery life calculation
					if ( this->lifeCurveNum == 0 && ! DataIPShortCuts::lAlphaFieldBlanks( 7 ) ) {
						ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
						ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 7 ) + '=' + DataIPShortCuts::cAlphaArgs( 7 ) );
						errorsFound = true;
					} else if ( DataIPShortCuts::lAlphaFieldBlanks( 7 ) ) {
						ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
						ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 7 ) + " cannot be blank when " + DataIPShortCuts::cAlphaArgs( 6 ) + " = Yes. But no entry found." );
						errorsFound = true;
					} else if ( ! InputProcessor::SameString( CurveManager::GetCurveType( this->lifeCurveNum ), "DoubleExponentialDecay" ) ) {
						ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
						ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 7 ) + '=' + DataIPShortCuts::cAlphaArgs( 7 ) );
						ShowContinueError( "Curve Type must be DoubleExponentialDecay but was " + CurveManager::GetCurveType( this->lifeCurveNum ) );
						errorsFound = true;
					}

					this->cycleBinNum = DataIPShortCuts::rNumericArgs( 14 );

					if ( ! errorsFound ) { // life cycle calculation for this battery, allocate arrays for degradation calculation
					//humm zero base instead of 1?
						this->b10.resize( this->maxRainflowArrayBounds + 2, 0.0 );
						this->x0.resize( this->maxRainflowArrayBounds + 2, 0 );
						this->nmb0.resize( this->cycleBinNum + 1, 0.0 );
						this->oneNmb0.resize( this->cycleBinNum+ 1, 0.0 );
					}
				}

				this->parallelNum          = DataIPShortCuts::rNumericArgs( 2 );
				this->seriesNum            = DataIPShortCuts::rNumericArgs( 3 );
				this->maxAhCapacity        = DataIPShortCuts::rNumericArgs( 4 );
				this->startingSOC          = DataIPShortCuts::rNumericArgs( 5 );
				this->availableFrac        = DataIPShortCuts::rNumericArgs( 6 );
				this->chargeConversionRate = DataIPShortCuts::rNumericArgs( 7 );
				this->chargedOCV           = DataIPShortCuts::rNumericArgs( 8 );
				this->dischargedOCV        = DataIPShortCuts::rNumericArgs( 9 );
				this->internalR            = DataIPShortCuts::rNumericArgs( 10 );
				this->maxDischargeI        = DataIPShortCuts::rNumericArgs( 11 );
				this->cutoffV              = DataIPShortCuts::rNumericArgs( 12 );
				this->maxChargeRate        = DataIPShortCuts::rNumericArgs( 13 );

				SetupOutputVariable( "Electric Storage Operating Mode Index []", this->storageMode, "System", "Average", this->name );
				SetupOutputVariable( "Electric Storage Charge State [Ah]", this->absoluteSOC, "System", "Average", this->name );
				SetupOutputVariable( "Electric Storage Charge Fraction []", this->fractionSOC, "System", "Average", this->name );
				SetupOutputVariable( "Electric Storage Total Current [A]", this->batteryCurrent, "System", "Average", this->name );
				SetupOutputVariable( "Electric Storage Total Voltage [V]", this->batteryVoltage, "System", "Average", this->name );

				if ( this->lifeCalculation == batteryLifeCalculationYes ) {
					SetupOutputVariable( "Electric Storage Degradation Fraction []", this->batteryDamage, "System", "Average", this->name );
				}
				break;
			}

			} // switch storage model type

			SetupOutputVariable( "Electric Storage Charge Power [W]", this->storedPower, "System", "Average", this->name  );
			SetupOutputVariable( "Electric Storage Charge Energy [J]", this->storedEnergy, "System", "Sum", this->name  );
			SetupOutputVariable( "Electric Storage Production Decrement Energy [J]", this->decrementedEnergyStored, "System", "Sum", this->name , _, "ElectricityProduced", "ELECTRICSTORAGE", _, "Plant" );
			SetupOutputVariable( "Electric Storage Discharge Power [W]", this->drawnPower, "System", "Average", this->name  );
			SetupOutputVariable( "Electric Storage Discharge Energy [J]", this->drawnEnergy, "System", "Sum", this->name , _, "ElectricityProduced", "ELECTRICSTORAGE", _, "Plant" );
			SetupOutputVariable( "Electric Storage Thermal Loss Rate [W]", this->thermLossRate, "System", "Average", this->name  );
			SetupOutputVariable( "Electric Storage Thermal Loss Energy [J]", this->thermLossEnergy, "System", "Sum", this->name  );
			if ( DataGlobals::AnyEnergyManagementSystemInModel ) {
				if ( this->storageModelMode == simpleBucketStorage ) {
					SetupEMSInternalVariable( "Electrical Storage Maximum Capacity", this->name , "[J]", this->maxEnergyCapacity );
				} else if ( this->storageModelMode == kiBaMBattery ) {
					SetupEMSInternalVariable( "Electrical Storage Maximum Capacity", this->name , "[Ah]", this->maxAhCapacity );
				}
				SetupEMSActuator( "Electrical Storage", this->name , "Power Draw Rate", "[W]", this->eMSOverridePelFromStorage, this->eMSValuePelFromStorage );
				SetupEMSActuator( "Electrical Storage", this->name , "Power Charge Rate", "[W]", this->eMSOverridePelIntoStorage, this->eMSValuePelIntoStorage );
			}

			if ( this->zoneNum > 0 ) {
				switch ( this->storageModelMode )
				{
				case simpleBucketStorage: {
					SetupZoneInternalGain( this->zoneNum, "ElectricLoadCenter:Storage:Simple", this->name , DataHeatBalance::IntGainTypeOf_ElectricLoadCenterStorageSimple, this->qdotConvZone, _, this->qdotRadZone );
					break;
				}
				case kiBaMBattery: {
					SetupZoneInternalGain( this->zoneNum, "ElectricLoadCenter:Storage:Battery", this->name , DataHeatBalance::IntGainTypeOf_ElectricLoadCenterStorageBattery, this->qdotConvZone, _, this->qdotRadZone );
					break;
				}

				} // switch storage model type

			}
		
		} else { // storage not found
			ShowSevereError( routineName + " did not find storage name = " + objectName);
			errorsFound = true;
		}
		if ( errorsFound ) {
			ShowFatalError( routineName + "Preceding errors terminate program." );
		}
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
		std::string objectName
	)
	{
		this->availSchedPtr = 0;
		this->usageMode = useNotYetSet;
		this->heatLossesDestination = heatLossNotDetermined;
		this->zoneNum = 0;
		this->zoneRadFrac = 0.0;
		this->ratedCapacity = 0.0;
		this->phase = 0;
		this->factorTempCoeff = 0.0;
		this->tempRise = 0.0;
		this->eddyFrac = 0.0;
		this->performanceInputMode = perfInputMethodNotSet;
		this->ratedEfficiency = 0.0;
		this->ratedPUL = 0.0;
		this->ratedTemp = 0.0;
		this->maxPUL = 0.0;
		this->considerLosses = true;
		this->ratedNL = 0.0;
		this->ratedLL = 0.0;
		this->loadCenterNum = 0;
		this->overloadErrorIndex = 0;
		this->efficiency = 0.0;
		this->powerIn = 0.0;
		this->energyIn = 0.0;
		this->powerOut = 0.0;
		this->energyOut = 0.0;
		this->noLoadLossRate = 0.0;
		this->noLoadLossEnergy = 0.0;
		this->loadLossRate = 0.0;
		this->loadLossEnergy = 0.0;
		this->thermalLossRate = 0.0;
		this->thermalLossEnergy = 0.0;
		this->elecUseUtility = 0.0;
		this->elecProducedCoGen = 0.0;
		this->qdotConvZone = 0.0;
		this->qdotRadZone = 0.0;
				
		std::string const routineName = "ElectricTransformer constructor ";
		int numAlphas; // Number of elements in the alpha array
		int numNums; // Number of elements in the numeric array
		int IOStat; // IO Status when calling get input subroutine
		bool errorsFound = false;
		int transformerIDFObjectNum = 0;
		DataIPShortCuts::cCurrentModuleObject = "ElectricLoadCenter:Transformer";

		transformerIDFObjectNum = InputProcessor::GetObjectItemNum( "ElectricLoadCenter:Transformer",  objectName );
		if ( transformerIDFObjectNum > 0 ) {
			InputProcessor::GetObjectItem( DataIPShortCuts::cCurrentModuleObject, transformerIDFObjectNum, DataIPShortCuts::cAlphaArgs, numAlphas, DataIPShortCuts::rNumericArgs, numNums, IOStat, DataIPShortCuts::lNumericFieldBlanks, DataIPShortCuts::lAlphaFieldBlanks, DataIPShortCuts::cAlphaFieldNames, DataIPShortCuts::cNumericFieldNames  );
			this->name  = DataIPShortCuts::cAlphaArgs( 1 );
			// how to verify names are unique across objects? add to GlobalNames?
			if ( DataIPShortCuts::lAlphaFieldBlanks( 2 ) ) {
				this->availSchedPtr = DataGlobals::ScheduleAlwaysOn;
			} else {
				this->availSchedPtr = ScheduleManager::GetScheduleIndex( DataIPShortCuts::cAlphaArgs( 2 ) );
				if ( this->availSchedPtr == 0 ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 2 ) + " = " + DataIPShortCuts::cAlphaArgs( 2 ) );
					errorsFound = true;
				}
			}

			if ( DataIPShortCuts::lAlphaFieldBlanks( 3 ) ) {
				this->usageMode = powerInFromGrid; //default
			} else if ( InputProcessor::SameString(DataIPShortCuts::cAlphaArgs( 3 ), "PowerInFromGrid" ) ) {
				this->usageMode = powerInFromGrid;
			} else if ( InputProcessor::SameString(DataIPShortCuts::cAlphaArgs( 3 ), "PowerOutFromOnsiteGeneration" ) ) {
				this->usageMode = powerOutFromBldg;
			} else {
					ShowWarningError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 3 ) + " = " + DataIPShortCuts::cAlphaArgs( 3 ) );
					errorsFound = true;
			}

			this->zoneNum = InputProcessor::FindItemInList( DataIPShortCuts::cAlphaArgs( 4 ), DataHeatBalance::Zone );
			if ( this->zoneNum > 0 ) this->heatLossesDestination = zoneGains;
			if ( this->zoneNum == 0 ) {
				if ( DataIPShortCuts::lAlphaFieldBlanks( 4 ) ) {
					this->heatLossesDestination = lostToOutside;
				} else {
					this->heatLossesDestination = lostToOutside;
					ShowWarningError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 4 ) + " = " + DataIPShortCuts::cAlphaArgs( 4 ) );
					ShowContinueError( "Zone name not found. Transformer heat losses will not be added to a zone" );
					// continue with simulation but storage losses not sent to a zone.
				}
			}
			this->zoneRadFrac   = DataIPShortCuts::rNumericArgs( 1 );
			this->ratedCapacity = DataIPShortCuts::rNumericArgs( 2 );
			this->phase         = DataIPShortCuts::rNumericArgs( 3 );

			if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 5 ), "Copper" ) ) {
				this->factorTempCoeff = 234.5;
			} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 5 ), "Aluminum" ) ) {
				this->factorTempCoeff = 225.0;
			} else {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
				ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 5 ) + " = " + DataIPShortCuts::cAlphaArgs( 5 ) );
				errorsFound = true;
			}
			this->tempRise = DataIPShortCuts::rNumericArgs( 4 );
			this->eddyFrac = DataIPShortCuts::rNumericArgs( 5 );

			if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "RatedLosses" ) ) {
				this->performanceInputMode = lossesMethod;
			} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 6 ), "NominalEfficiency" ) ) {
				this->performanceInputMode = efficiencyMethod;
			} else {
				ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" +  DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
				ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 6 ) + " = " + DataIPShortCuts::cAlphaArgs( 6 ) );
				errorsFound = true;
			}

			this->ratedNL         = DataIPShortCuts::rNumericArgs( 6 );
			this->ratedLL         = DataIPShortCuts::rNumericArgs( 7 );
			this->ratedEfficiency = DataIPShortCuts::rNumericArgs( 8 );
			this->ratedPUL        = DataIPShortCuts::rNumericArgs( 9 );
			this->ratedTemp       = DataIPShortCuts::rNumericArgs( 10 );
			this->maxPUL          = DataIPShortCuts::rNumericArgs( 11 );
			//Check the input for MaxPUL if the performance input method is EfficiencyMethod
			if ( this->performanceInputMode == efficiencyMethod ) {
				if ( DataIPShortCuts::lNumericFieldBlanks( 11 ) ) {
					this->maxPUL = this->ratedPUL;
				} else if ( this->maxPUL <= 0 || this->maxPUL > 1 ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cNumericFieldNames( 11 ) + "=[" + General::RoundSigDigits( DataIPShortCuts::rNumericArgs( 11 ), 3 ) + "]." );
					ShowContinueError( "Entered value must be > 0 and <= 1." );
					errorsFound = true;
				}
			}
			if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 7 ), "Yes" ) ) {
				this->considerLosses = true;
			} else if ( InputProcessor::SameString( DataIPShortCuts::cAlphaArgs( 7 ), "No" ) ) {
				this->considerLosses = false;
			} else {
				if ( this->usageMode == powerInFromGrid ) {
					ShowSevereError( routineName + DataIPShortCuts::cCurrentModuleObject + "=\"" + DataIPShortCuts::cAlphaArgs( 1 ) + "\", invalid entry." );
					ShowContinueError( "Invalid " + DataIPShortCuts::cAlphaFieldNames( 7 ) + " = " + DataIPShortCuts::cAlphaArgs( 7 ) );
					errorsFound = true;
				}
			}

			int numAlphaBeforeMeter = 7;
			int numWiredMeters = numAlphas - numAlphaBeforeMeter;

			if ( this->usageMode == powerInFromGrid ) {

				//Provide warning if no meter is wired to a transformer used to get power from the grid
				if ( numWiredMeters <= 0 ) {
					ShowWarningError( routineName + "ElectricLoadCenter:Transformer=\"" + this->name + "\":" );
					ShowContinueError( "ISOLATED Transformer: No meter wired to a transformer used to input power from grid" );
				}

				this->wiredMeterNames.resize( numWiredMeters, "" );
				this->wiredMeterPtrs.resize( numWiredMeters, 0 );
				this->specialMeter.resize( numWiredMeters, false );

				//Meter check deferred because they may have not been "loaded" yet,
				for ( auto loopCount = 1; loopCount <= numWiredMeters; ++loopCount ) {
					this->wiredMeterNames[ loopCount ] = InputProcessor::MakeUPPERCase( DataIPShortCuts::cAlphaArgs( loopCount + numAlphaBeforeMeter ) );
					//Assign SpecialMeter as TRUE if the meter name is Electricity:Facility or Electricity:HVAC
					if ( InputProcessor::SameString( this->wiredMeterNames[ loopCount ], "Electricity:Facility" ) || InputProcessor::SameString( this->wiredMeterNames[ loopCount ], "Electricity:HVAC" ) ) {
						this->specialMeter[ loopCount ] = true;
					} else {
						this->specialMeter[ loopCount ] = false;
					}
				}
			}
			SetupOutputVariable( "Transformer Efficiency []", this->efficiency, "System", "Average", this->name );
			SetupOutputVariable( "Transformer Input Electric Power [W]", this->powerIn, "System", "Average", this->name );
			SetupOutputVariable( "Transformer Input Electric Energy [J]", this->energyIn, "System", "Sum", this->name );
			SetupOutputVariable( "Transformer Output Electric Power [W]", this->powerOut, "System", "Average", this->name );
			SetupOutputVariable( "Transformer Output Electric Energy [J]", this->energyOut, "System", "Sum", this->name );
			SetupOutputVariable( "Transformer No Load Loss Rate [W]", this->noLoadLossRate, "System", "Average", this->name );
			SetupOutputVariable( "Transformer No Load Loss Energy [J]", this->noLoadLossEnergy, "System", "Sum", this->name );
			SetupOutputVariable( "Transformer Load Loss Rate [W]", this->loadLossRate, "System", "Average", this->name );
			SetupOutputVariable( "Transformer Load Loss Energy [J]", this->loadLossEnergy, "System", "Sum", this->name );
			SetupOutputVariable( "Transformer Thermal Loss Rate [W]", this->thermalLossRate, "System", "Average", this->name );
			SetupOutputVariable( "Transformer Thermal Loss Energy [J]", this->thermalLossEnergy, "System", "Sum", this->name );
			SetupOutputVariable( "Transformer Distribution Electric Loss Energy [J]", this->elecUseUtility, "System", "Sum", this->name, _, "Electricity", "ExteriorEquipment", "Transformer", "System" );
			SetupOutputVariable( "Transformer Cogeneration Electric Loss Energy [J]", this->elecProducedCoGen, "System", "Sum", this->name, _, "ElectricityProduced", "COGENERATION", _, "System" );

			if ( this->zoneNum > 0 ) {
				SetupZoneInternalGain( this->zoneNum, "ElectricLoadCenter:Transformer", this->name, DataHeatBalance::IntGainTypeOf_ElectricLoadCenterTransformer, this->qdotConvZone, _, this->qdotRadZone );
			}

		} else {
			ShowSevereError( routineName + " did not find transformer name = " + objectName);
			errorsFound = true;
		
		
		}

		if ( errorsFound ) {
			ShowFatalError( routineName + "Preceding errors terminate program." );
		}

	}

	void
	ElectricTransformer::manageTransformers()
	{
	
	}

	void
	ElectricTransformer::setupMeterIndices()
	{
		if (this->usageMode == powerInFromGrid ) {
			for ( auto meterNum = 1; meterNum <= this->wiredMeterNames.size(); ++meterNum ) {

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
