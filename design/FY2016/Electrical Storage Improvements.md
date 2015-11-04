Electric Storage Controls 
================

**B. Griffith, Energy Archmage Company, for DOE/NREL/GARD**

 - November 9, 2015
 - 
 

## Justification for New Feature ##

Electric storage capabilities in EnergyPlus are too limited. The current capabilities for storage were only intended for storing electrical power produced by on-site generators and the charge and discharge supervisory controls are hard-coded with no options.  The improvements planned will address the following list of applications for electric storage that EnergyPlus is not currently able to model:

- Electric storage charge power is taken from the grid with no on-site generation.  
- Electric storage charge power combines grid-supplied electricity with on-site generation.
- Electric storage charge power uses all of the on-site generation regardless of facility demand.
- Electric storage discharge controlled to regulate facility demand on utility service.
- Electric storage discharge controlled to follow a prescribed schedule
- Electric storage discharge controlled to follow a prescribed meter
- Electric storage discharge controlled to regulate net export to the grid for time-of-day net metering or distribution grid voltage regulation.

In addition, EnergyPlus lacks EMS actuators that are needed to allow users to write their own custom supervisory control programs using the EMS.

New capabilities to handle grid-connected storage and more versatile storage capabilities are justified to allow users to model such systems and to compare the performance of thermal energy storage to electric power storage. 

## E-mail and  Conference Call Conclusions ##

<insert text>

## Overview ##

The scope of intended applications for the ElectricLoadCenter in EnergyPlus will be expanded to include grid interactions in addition to expanding supervisory control options for electric storage. Controls over storage will be expanded while retaining legacy intended behavior as default. A collection of changes and bug fixes will be addressed to improve electrical storage.  EMS actuators will be added to facilitate custom control beyond what will be available with native methods. 

## Approach ##

The input object ElectricLoadCenter:Distribution will be expanded to add new input fields to improve control over charging and discharging of electrical storage. 

New EMS actuators will be added. 

A number of known issues will be address as part of a general rewrite, including:
	1. Issue #3211 -- capacity limited to on-site generator charging.
	2. issue #5004 -- battery won't charge at its max rate
	3. Issue #4273 -- documentation of storage rules and behavior. 
	4. issue #3531 -- documentation of tariffs with on-site production, storage etc. Change example files to use meters and tariffs as currently intended.

## Testing/Validation/Data Sources ##

insert text

## Input Output Reference Documentation ##

Introductory text will be added at the Group level to help explain how the various ElectricLoadCenter:* and Generator:* objects relate to one another to provide higher level guidance. 

ElectricLoadCenter:Distribution object will be modified to add the following new input fields at the end of the object.  All of the new objects will be setup so if they are omitted the legacy behavior, or at least intended behavior where there no issues, will be retained. 

#### Field: Storage Charge Operation Scheme
This field is used to determine which power source is used to charge the electric storage device. There are four choices: OnSiteGenerators, OnSiteGeneratorsSurplus, ScheduledGridSupply, and OnSiteGeneratorSurplusPlusScheduledGridSupply. 

- OnSiteGenerators key choice indicates that all the power produced on-site is used to charge the storage regardless of the current facility power demand. The rate of charging will depend on how the generators are controlled.
- OnSiteGeneratorsSurplus key choice indicates that the power produced on-site that exceeds the facility demand is used to charge the storage.  The rate of charging will depend on how the generators are controlled and the current level of power consumed by the building and its systems. This was the intended legacy behavior prior to version 8.5 and is therefore the default. 
- ScheduledGridSupply key choice indicates that the power used to charge will be drawn from the utility service connection.  The rate of charging will depend on the power level set in the input field Maximum Storage Charge Grid Supply Power as modified by a fractional schedule named in the input field called Storage Charge Grid Supply Power Fraction Schedule Name. This is the only choice available when electric storage is used without any on-site generators. If generators are present they will not be used to charge storage. 
- OnSiteGeneratorSurplusPlusScheduledGridSupply key choice indicates that the power used to charge will be drawn from both utility service connection and what is produced on-site that exceeds the facility demand.  The rate of charging will depend on the power level set in the input field Maximum Storage Charge Grid Supply Power as modified by a fractional schedule named in the input field called Storage Charge Grid Supply Power Fraction Schedule Name, and on how the generators are controlled and the current level of power consumed by the building and its systems. 

####Field: Maximum Storage State of Charge Fraction
This numeric field specifies the fraction of storage capacity used as upper limit for controlling charging.  Charging will be constrained so that charging will stop the once this limit is reached. This fraction is the state of charge of the storage device where 1.0 is completely full and 0.0 is completely empty.  This allows supervisory control over charging to model behavior intended to protect the battery from damage. The legacy behavior prior to version 8.5 was to charge to full capacity and therefore the default is 1.0. 

####Field: Minimum Storage State of Charge Fraction
This numeric field specifies the fraction of storage capacity used as lower limit for controlling discharging.  Discharging will be constrained so that discharging will stop once this limit is reached.  This fraction is the state of charge of the storage device where 1.0 is completely full and 0.0 is completely empty.  This allows supervisory control over discharging to model behavior intended to protect the battery from damage. The legacy behavior prior to version 8.5 was to discharge to empty and therefore the default is 0.0.  

#### Field: Maximum Storage Charge Grid Supply Power
This numeric field specifies the maximum rate that electric power taken from grid to charge storage, in Watts.  This field is used when the Storage Charge Operation Scheme is set to ScheduledGridSupply or OnSiteGeneratorSurplusPlusScheduledGridSupply. The actual rate of utility service draw will be this value multiplied by the value in the schedule named in the following field.  This is the power level as viewed from grid. If a transformer and/or rectifier is involved the power actually going into storage will be adjusted downward as a result of conversion losses.    

#### Field: Storage Charge Grid Supply Power Fraction Schedule Name
This field is the name of a schedule that is used to control the timing and magnitude of charging from grid supply. This field is only used, and is required, when Storage Charge Operation Scheme is set to ScheduledGridSupply or OnSiteGeneratorSurplusPlusScheduledGridSupply. Schedule values should be fractions between 0.0 and 1.0, inclusive, and are multiplied by the grid supply power in the previous field to control the rate of charging from the grid. 

#### Field: Storage Converter Name
 This field is the name of an ElectricLoadCenter:Storage:Converter object defined elsewhere in the input file that describes the performance of converting convert AC to DC when charging DC storage from grid supply. This field is required when using DC storage (buss type DirectCurrentWithInverterDCStorage) with grid supplied charging power (Storage Charge Operation Scheme is set to ScheduledGridSupply or OnSiteGeneratorSurplusPlusScheduledGridSupply.) Although some inverter devices are bidirectional a separate converter object is needed to describe AC to DC performance. 

#### Field Storage Discharge Operation Scheme
This field is used to determine how storage discharge is controlled.  There are five choices:  FacilityDemandLimit, TrackFacilityElectricDemand, TrackSchedule, TrackMeter, and ScheduledGridExport.

- FacilityDemandLimit indicates that storage discharge control will limit facility power demand drawn from the utility service while accounting for any on-site generation.  The rate of discharge will depend on the current level of power consumed by the building and its systems, the power generated any on-site generation as well as demand limit and schedule in the next two fields. 
- TrackFacilityElectricDemand indicates that storage discharge control will follow the facility power demand while accounting for any on-site generation.  This was the intended behavior prior to version 8.5 and is therefore the default. 
- TrackSchedule indicates that the storage discharge control will follow the schedule named in the input field called Storage Discharge Track Schedule Name.  
- TrackMeter TrackMeter indicates that storage discharge control will follow an electric meter named in the field called Storage Discharge Track Meter Name.
- ScheduledGridExport ScheduledGridExport indicates that storage discharge control will export power to the utility service connection following the power level in the field called Maximum Storage Discharge Grid Export Power multiplied by the value in the schedule named in the field Storage Discharge Grid Export Fraction Schedule Name.

#### Field: Storage Discharge Purchased Electric Demand Limit
This field is the target demand limit used to control storage discharge when using the required field for FacilityDemandLimit discharge operation scheme
       \type real
       \units W
 
#### Field: Storage Discharge Purchased Electric Demand Limit Fraction Schedule Name

#### Field: Storage Discharge Track Schedule Name
       \note required when Generator Operation Scheme Type=TrackSchedule
       \note schedule values in Watts
       \type object-list
       \object-list ScheduleNames
  A14, \field Storage Discharge Track Meter Name
       \note required when Generator Operation Scheme Type=TrackMeter
       \type external-list
       \external-list autoRDDmeter
  N4 , \field Maximum Storage Discharge Grid Export Power
       \note Maximum rate that electric power can be fed to grid from storage discharge
       \type real
       \units W
       
  A15; \field Storage Discharge Grid Export Fraction Schedule Name
       \note controls timing and magnitude of discharging to grid
       \note required 


## Input Description ##

insert text

## Outputs Description ##

insert text

## Engineering Reference ##

insert text

## Example File and Transition Changes ##

insert text

## References ##

insert text



