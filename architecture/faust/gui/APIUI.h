#ifndef API_UI_H
#define API_UI_H

#include "faust/misc.h"
#include "faust/gui/meta.h"
#include "faust/gui/PathUI.h"
#include "faust/gui/ValueConverter.h"
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

enum { kLin = 0, kLog = 1, kExp = 2 };

class APIUI : public PathUI, public Meta
{
    protected:
    
        int	fNumParameters;
        vector<string>			fName;
        map<string, int>		fMap;
        vector<ValueConverter*>	fConversion;
        vector<FAUSTFLOAT*>		fZone;
        vector<FAUSTFLOAT>		fInit;
        vector<FAUSTFLOAT>		fMin;
        vector<FAUSTFLOAT>		fMax;
        vector<FAUSTFLOAT>		fStep;        
        vector<string>			fUnit; 
        vector<ZoneControl*>	fAcc[3]; 
    
        // Current values controlled by metadata
        string	fCurrentUnit;     
        int     fCurrentScale;
        string	fCurrentAcc;     

        // Add a generic parameter
        virtual void addParameter(const char* label, 
                                FAUSTFLOAT* zone, 
                                FAUSTFLOAT init, 
                                FAUSTFLOAT min, 
                                FAUSTFLOAT max, 
                                FAUSTFLOAT step)
        {
            string name = buildPath(label);

            fMap[name] = fNumParameters++;
            fName.push_back(name);
            fZone.push_back(zone);
            fInit.push_back(init);
            fMin.push_back(min);
            fMax.push_back(max);
            fStep.push_back(step);

            //handle unit metadata
            fUnit.push_back(fCurrentUnit); 
            fCurrentUnit = "";

            //handle scale metadata
            switch (fCurrentScale) {
                case kLin : fConversion.push_back(new LinearValueConverter(0,1, min, max)); break;
                case kLog : fConversion.push_back(new LogValueConverter(0,1, min, max)); break;							
                case kExp : fConversion.push_back(new ExpValueConverter(0,1, min, max)); break;
            }
            fCurrentScale  = kLin;
        
            // handle acc metadata "...[acc : <axe> <curve> <amin> <amid> <amax>]..."
            if (fCurrentAcc.size() > 0) {
                istringstream iss(fCurrentAcc); 
                int axe, curve;
                double amin, amid, amax;
                iss >> axe >> curve >> amin >> amid >> amax;
                
                if ((0 <= axe) && (axe < 3) && 
                    (0 <= curve) && (curve < 4) &&
                    (amin < amax) && (amin <= amid) && (amid <= amax)) 
                {
                    fAcc[axe].push_back(new CurveZoneControl(zone, amin, amid, amax, min, init, max));
                } else {
                    cerr << "incorrect acc metadata : " << fCurrentAcc << endl;
                }
            }
            fCurrentAcc = "";
        }
    
        int getAccZoneIndex(int p, int acc)
        {
            FAUSTFLOAT* zone = fZone[p];
            for (int i = 0; i < fAcc[acc].size(); i++) {
                if (zone == fAcc[acc][i]->getZone()) return i;
            }
            return -1;
        }
    
     public:
    
        APIUI() : fNumParameters(0) {}
        virtual ~APIUI()
        {
            vector<ValueConverter*>::iterator it1;
            for (it1 = fConversion.begin(); it1 != fConversion.end(); it1++) {
                delete(*it1);
            }
            
            vector<ZoneControl*>::iterator it2;
            for (it2 = fAcc[0].begin(); it2 != fAcc[0].end(); it2++) {
                delete(*it2);
            }
            for (it2 = fAcc[1].begin(); it2 != fAcc[1].end(); it2++) {
                delete(*it2);
            }
            for (it2 = fAcc[2].begin(); it2 != fAcc[2].end(); it2++) {
                delete(*it2);
            }
        }

        // -- widget's layouts
    
        virtual void openTabBox(const char* label)			{ fControlsLevel.push_back(label); 	}    
        virtual void openHorizontalBox(const char* label)	{ fControlsLevel.push_back(label); 	} 
        virtual void openVerticalBox(const char* label)		{ fControlsLevel.push_back(label); 	}
        virtual void closeBox()								{ fControlsLevel.pop_back();		}
    
        // -- active widgets

        virtual void addButton(const char* label, FAUSTFLOAT* zone)
        {
            addParameter(label, zone, 0, 0, 1, 1);
        }
    
        virtual void addCheckButton(const char* label, FAUSTFLOAT* zone)
        {
            addParameter(label, zone, 0, 0, 1, 1);
        }
    
        virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
        {
            addParameter(label, zone, init, min, max, step);
        }
    
        virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
        {
            addParameter(label, zone, init, min, max, step);
        }
    
        virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
        {
            addParameter(label, zone, init, min, max, step);
        }

        // -- passive widgets
    
        virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) 
        {
            addParameter(label, zone, min, min, max, (max-min)/1000.0);
        }
    
        virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
        {
            addParameter(label, zone, min, min, max, (max-min)/1000.0);
        }

        // -- metadata declarations

        virtual void declare(FAUSTFLOAT* zone, const char* key, const char* val)
        {
			if (strcmp(key, "scale") == 0) {
                if (strcmp(val, "log") == 0) {
                    fCurrentScale = kLog;
                } else if (strcmp(val, "exp") == 0) {
                    fCurrentScale = kExp;
                } else {
                    fCurrentScale = kLin;
                }
			} else if (strcmp(key, "unit") == 0) {
				fCurrentUnit = val;
			} else if (strcmp(key, "acc") == 0) {
				fCurrentAcc = val;
			}
        }

        virtual void declare(const char* key, const char* val)
        {}

		//-------------------------------------------------------------------------------
		// Simple API part
		//-------------------------------------------------------------------------------
		int getParamsCount()				{ return fNumParameters; }
		int getParamIndex(const char* n) 	{ return (fMap.count(n)>0) ? fMap[n] : -1; }
		const char* getParamName(int p)		{ return fName[p].c_str(); }
		const char* getParamUnit(int p)		{ return fUnit[p].c_str(); }
		float getParamMin(int p)			{ return fMin[p]; }
		float getParamMax(int p)			{ return fMax[p]; }
		float getParamStep(int p)			{ return fStep[p]; }
	
		float getParamValue(int p)			{ return *fZone[p]; }
		void setParamValue(int p, float v)	{ *fZone[p] = v; }
	
		float getParamRatio(int p)			{ return fConversion[p]->faust2ui(*fZone[p]); }
		void setParamRatio(int p, float r)	{ *fZone[p] = fConversion[p]->ui2faust(r); }
	
		float value2ratio(int p, float r)	{ return fConversion[p]->faust2ui(r); }
		float ratio2value(int p, float r)	{ return fConversion[p]->ui2faust(r); }
    
        // acc in [0, 1, 2]
        void propagateAcc(int acc, double a)
        {
            for (int i = 0; i < fAcc[acc].size(); i++) {
                fAcc[acc][i]->update(a);
            }
        }

        void setAccConverter(int p, int acc, int curve, double amin, double amid, double amax)
        {
            int id1 = getAccZoneIndex(p, 0);
            int id2 = getAccZoneIndex(p, 1);
            int id3 = getAccZoneIndex(p, 2);
            
            // Deactivates everywhere..
            if (id1 != -1) fAcc[0][id1]->setActive(false);
            if (id2 != -1) fAcc[1][id2]->setActive(false);
            if (id3 != -1) fAcc[2][id3]->setActive(false);
            
            if (acc == -1) { // Means: no more mapping...
                // So stay all deactivated...
            } else {
                int id4 = getAccZoneIndex(p, acc);
                if (id4 != -1) {
                    // Reactivate the one we edit...
                    fAcc[acc][id4]->update(curve, amin, amid, amax, fMin[p], fInit[p], fMax[p]);
                    fAcc[acc][id4]->setActive(true);
                } else {
                    // Allocate a new CurveZoneControl which is activated by default
                    FAUSTFLOAT* zone = fZone[p];
                    fAcc[acc].push_back(new CurveZoneControl(zone, amin, amid, amax, fMin[p], fInit[p], fMax[p]));
                    __android_log_print(ANDROID_LOG_ERROR, "Faust", "setAccConverter new CurveZoneControl %d", acc);
                }
            }
         }
   
};

#endif