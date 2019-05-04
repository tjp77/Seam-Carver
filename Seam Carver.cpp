#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <omp.h>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;


struct Pixel
{
	short grey, red, green, blue;
	
	Pixel ()
	{  } 

	Pixel(short r, short g, short b) : red(r), green(g), blue(b)
	{  }
};


// Util.
void LoadData(string fileName, vector<vector<Pixel>> &imageMap, vector<vector<short>> &energyMap, string &fileHeading, short &width, short &height, short &maxVal);
void LoadGreyPixels(std::ifstream &ifs, vector<vector<Pixel>> &imageMap, const short &width, const short &height);
void LoadColorPixels(std::ifstream &ifs, vector<vector<Pixel>> &imageMap, const short &width, const short &height);
void stringSplit(string stringtoSplit, string (&stringParts)[2] , char delim);
void SaveModifiedImage(string fileName, vector<vector<Pixel>> imageMap, string fileHeading, short &width, short &height, short maxVal, bool transposed);
short FindMin(const short &num1, const short &num2);
short FindMin(const short &num1, const short &num2, const short &num3);

// Seam Carve.
void GenerateEnergyMap(vector<vector<Pixel>> imageMap, vector<vector<short>> &energyMap, const short &width, const short &height);
void GenerateColorEnergyMap(vector<vector<Pixel>> imageMap, vector<vector<short>> &energyMap, const short &width, const short &height);
void GenerateCumalativeEnergyMap(vector<vector<short>> &cumalativeEnergyMap, const short &width, const short &height);
void RemoveVertSeam(vector<vector<Pixel>> &imageMap, const vector<vector<short>> &cumalativeEnergyMap, const short &width, const short &height);
void Transpose(const vector<vector<Pixel>> &map, vector<vector<Pixel>> &transposedMap, const short &width, const short &height);
void InitEnergyMap(vector<vector<short>> &energyMap, const short &width, const short &height);

// Generate Enery Map vrsn Pointer
void (*energyAlg)(vector<vector<Pixel>> imageMap, vector<vector<short>> &energyMap, const short &width, const short &height);


bool isColor = false;

int main(int argc, char* argv[])  // Execute => Parameters. 
{
	string fileHeading = "", fileName, outputFile = "image_processed.pgm"; 
	short width, height, maxVal;
	bool transposePrint = false;
    
    fileName = argv[1];
    
    // Check if dealing with a color image or not. 
    if (fileName.find(".ppm") != string::npos) 
	{ 
		isColor = true;
		outputFile = "image_processed.ppm";
	} 
    
    int vertSeamCount = atoi(argv[2]), horizSeamCount = atoi(argv[3]); 
	
	vector<vector<Pixel>> imageMap;
    vector<vector<short>> energyMap;

    LoadData(fileName, imageMap, energyMap, fileHeading, width, height, maxVal); 
    
    if (isColor)
    { energyAlg = GenerateColorEnergyMap; }
	else
	{ energyAlg = GenerateEnergyMap; }
	
	// Do vertical seams.
	for (int i = 0; i < vertSeamCount; i++)
	{
		energyAlg(imageMap, energyMap, width, height);

	    //Energy map will become the cumalative energy map. 
    	GenerateCumalativeEnergyMap(energyMap, width, height);

		RemoveVertSeam(imageMap, energyMap, width, height); 
		--width;
	}

	// Do horizontal seams.
	vector<vector<Pixel>> transposedMap(width, vector<Pixel>(height)); 

	if (horizSeamCount > 0)
	{ 
		Transpose(imageMap, transposedMap, width, height);
		short temp = width;
		width = height;
		height = temp;

		if (vertSeamCount > 0)
		{ energyMap.clear(); }
		
		InitEnergyMap(energyMap, width, height);
		
		for (int i = 0; i < horizSeamCount; i++) 
		{ 	
			energyAlg(transposedMap, energyMap, width, height); 

	    	//Energy map will become the cumalative energy map. 
    		GenerateCumalativeEnergyMap(energyMap, width, height);

			RemoveVertSeam(transposedMap, energyMap, width, height); 
			--width;
		}
	
		// Save result after a transpose, saving back in correct orientation.
		SaveModifiedImage(outputFile, transposedMap, fileHeading, width, height, maxVal, true);
	}
	else // Standard save. 
	{ SaveModifiedImage(outputFile, imageMap, fileHeading, width, height, maxVal, false); }	

    cout << "/// Image Processed ///" << endl;
    
    return 0;
}

void LoadData(string fileName, vector<vector<Pixel>> &imageMap, vector<vector<short>> &energyMap, string &fileHeading, short &width, short &height, short &maxVal)
{
    std::ifstream ifs;
    ifs.open(fileName);

    if (ifs.is_open() == false)
    { cout << "Error, " << fileName << " could not be opened." << endl; }
    else
    {
        string line, stringParts[2];

		// File designation.
        getline(ifs, line);
        fileHeading += (line + "\n"); 
    	
        getline(ifs, line);
        
        // If there's a comment line, deal with it and then do w ^ h. 
        // Use of width and height variables over vector sizes for increased readability. 
        if (line[0] == '#')
		{ 
			fileHeading += (line + "\n");
			
			ifs >> width; 
			ifs >> height; 
	    }
	    else // No comment, just get w ^ h. 
	    {
	    	stringSplit(line, stringParts, ' ');
	    	
	    	width = stoi(stringParts[0]);
	    	height = stoi(stringParts[1]);
		}
		
		InitEnergyMap(energyMap, width, height);
		
		// Max value size.
        ifs >> maxVal;
		      
        short inputValue;
		imageMap.resize(height);
		
		if (isColor)
		{
			LoadColorPixels(ifs, imageMap, width, height);
		}
		else
		{ LoadGreyPixels(ifs, imageMap, width, height); }

    }

}

void LoadGreyPixels(std::ifstream &ifs, vector<vector<Pixel>> &imageMap, const short &width, const short &height)
{
	for (short i = 0; i < height; i++)
	{
		for (short j = 0; j < width; j++)
		{
			Pixel p;
		
			ifs >> p.grey;
			imageMap[i].push_back(p);
		}
	}
}

void LoadColorPixels(std::ifstream &ifs, vector<vector<Pixel>> &imageMap, const short &width, const short &height)
{
	for (short i = 0; i < height; i++)
	{
		for (short j = 0; j < width; j++)
		{
			short r, g, b;
			ifs >> r;
			ifs >> g;
			ifs >> b;
			
			Pixel p(r, g, b);
			imageMap[i].push_back(p);
		}
	}
}

void stringSplit(string stringtoSplit, string (&stringParts)[2], char delim)
{
    short j = 0;

    for (short i = 0; i < stringtoSplit.length(); i++)
    {
        if (stringtoSplit[i] != delim)
        {
           stringParts[j] += (char)stringtoSplit[i];
        }
        else
        { j++; }
    }
}

void SaveModifiedImage(string fileName, vector<vector<Pixel>> imageMap, string fileHeading, short &width, short &height, short maxVal, bool transposed)
{
	std::ofstream ofs;
	ofs.open(fileName);
	
	if (ofs.is_open() == false)
	{ cout << "Error: Output File Could Not Be Created." << endl; }
	
	ofs << fileHeading;
	
	if (transposed == false)
	{
		ofs << width << " " << height << "\n" << maxVal << "\n";
		
		for (short i = 0; i < imageMap.size(); i++)
		{
			for (short j = 0; j < imageMap[0].size(); j++) 
			{
				if (isColor)
				{
					ofs << imageMap[i][j].red << " ";
					ofs << imageMap[i][j].green << " ";
					ofs << imageMap[i][j].blue << " ";
				}
				else
				{ ofs << imageMap[i][j].grey; }
			
				if (j != width - 1)
				{ ofs << " "; }
			}
		
			ofs << "\n";
		}
	}
	else // Save a transposed image as restored to its original orientaion. 
	{		
		ofs << height << " " << width << "\n" << maxVal << "\n";
		
		for (short i = 0; i < imageMap[0].size(); i++)
		{
			for (short j = 0; j < imageMap.size(); j++) 
			{
				
				if (isColor)
				{
					ofs << imageMap[j][i].red << " ";
					ofs << imageMap[j][i].green<< " ";
					ofs << imageMap[j][i].blue<< " ";
				}
				else
				{ ofs << imageMap[j][i].grey; }

				if (j != height - 1)
				{ ofs << " "; }
			}
		
			ofs << "\n";
		}
	}
}

short FindMin(const short &num1, const short &num2)
{
	if (num1 <= num2) 
		return num1;
		
	return num2;
}

short FindMin(const short &num1, const short &num2, const short &num3)
{
	short firstMin = FindMin(num1, num2);
	
	if (firstMin <= num3)
		return firstMin;
		
	return num3;
}


void GenerateEnergyMap(vector<vector<Pixel>> imageMap, vector<vector<short>> &energyMap, const short &width, const short &height)
{	
	#pragma omp parallel for
	for (short i = 0; i < height; i++)
	{
		for (short j = 0; j < width; j++)
		{
			int energy = 0;

			// If not on an image edge and lacking a neighboring pixel
			// to that side... add difference between self and that 
			// neighbor to own energy level.		
			if (i > 0)
			{ energy += abs(imageMap[i][j].grey - imageMap[i - 1][j].grey); }
			
			if (i < height - 1)
			{ energy += abs(imageMap[i][j].grey - imageMap[i + 1][j].grey); }
			
			if (j > 0)
			{ energy += abs(imageMap[i][j].grey - imageMap[i][j - 1].grey); }
			
			if (j < width - 1)
			{ energy += abs(imageMap[i][j].grey - imageMap[i][j + 1].grey); }

			energyMap[i][j] = energy; 
		}
	}
	
}

void GenerateColorEnergyMap(vector<vector<Pixel>> imageMap, vector<vector<short>> &energyMap, const short &width, const short &height)
{	
	#pragma omp parallel for
	for (short i = 0; i < height; i++)
	{
		for (short j = 0; j < width; j++)
		{
			int energy = 0;

			// If not on an image edge and lacking a neighboring pixel
			// to that side... add difference between self and that 
			// neighbor to own energy level.		
			if (i > 0)
			{ 
				energy += abs(imageMap[i][j].red - imageMap[i - 1][j].red);
				energy += abs(imageMap[i][j].blue - imageMap[i - 1][j].blue);
				energy += abs(imageMap[i][j].green - imageMap[i - 1][j].green);
			}
			
			if (i < height - 1)
			{ 
				energy += abs(imageMap[i][j].red - imageMap[i + 1][j].red); 
				energy += abs(imageMap[i][j].blue - imageMap[i + 1][j].blue); 
				energy += abs(imageMap[i][j].green - imageMap[i + 1][j].green); 
			}
			
			if (j > 0)
			{ 
				energy += abs(imageMap[i][j].red - imageMap[i][j - 1].red); 
				energy += abs(imageMap[i][j].blue - imageMap[i][j - 1].blue); 
				energy += abs(imageMap[i][j].green - imageMap[i][j - 1].green); 
			}
			
			if (j < width - 1)
			{ 
				energy += abs(imageMap[i][j].red - imageMap[i][j + 1].red); 
				energy += abs(imageMap[i][j].blue - imageMap[i][j + 1].blue);
				energy += abs(imageMap[i][j].green - imageMap[i][j + 1].green);
			}

			energyMap[i][j] = energy; 
		}
	}
	
}


void GenerateCumalativeEnergyMap(vector<vector<short>> &cumalativeEnergyMap, const short &width, const short &height)
{	
	for (short i = 1, lasti = 0; i < height; i++, lasti++)
	{
		// We know that j = 0 and j = width - 1 are edge cases and need to be set differently, so 
		// will do them later and not have to check if j = 0 or j = (width - 1) every iteration
		// before computing a NE or NW neighbor. Would be lots of exra checks for a large image. 
		for (short j = 1; j < width - 1; j++) 
		{ 
			short fromNW = cumalativeEnergyMap[i][j] + cumalativeEnergyMap[lasti][j - 1];
			short fromN = cumalativeEnergyMap[i][j] + cumalativeEnergyMap[lasti][j];
			short fromNE = cumalativeEnergyMap[i][j] + cumalativeEnergyMap[lasti][j + 1];
			
			cumalativeEnergyMap[i][j] = FindMin(fromNW, fromN, fromNE);
		}
		
		// Set the edge cases for this row.
		short fromN = cumalativeEnergyMap[i][0] + cumalativeEnergyMap[lasti][0];
		short fromNE = cumalativeEnergyMap[i][0] + cumalativeEnergyMap[lasti][1];
		
		cumalativeEnergyMap[i][0] = FindMin(fromN,fromNE);
		
		fromN = cumalativeEnergyMap[i][width - 1] + cumalativeEnergyMap[lasti][width - 1];
		short fromNW = cumalativeEnergyMap[i][width - 1] + cumalativeEnergyMap[lasti][width - 2];
		
		cumalativeEnergyMap[i][width - 1] = FindMin(fromNW, fromN);
	}
	
}


void RemoveVertSeam(vector<vector<Pixel>> &imageMap, const vector<vector<short>> &cumalativeEnergyMap, const short &width, const short &height)
{
	short row = height - 1;
	short minValue = cumalativeEnergyMap[row][0], minXLoc = 0;
	
	// Go through whole bottom row to find lowest of it.
	for (short j = 1; j < width; j++)
	{
		if (cumalativeEnergyMap[row][j] < minValue)
		{ 
			minValue = cumalativeEnergyMap[row][j];
			minXLoc = j;
		} 
	}

	// Remove the first/starting cell of the seam from the image.
	imageMap[row].erase(imageMap[row].begin() + minXLoc);

	// Find each subsequent seam cell from there on.
	for (short i = height - 1; i > 0; i--)
	{   
		short toNW = SHRT_MAX; 
		short toN = cumalativeEnergyMap[i - 1][minXLoc];
		short toNE = SHRT_MAX; 
		short lowest = SHRT_MAX, locOffset = 0;
		
		// Set if not an edge case which doens't have...
		if (minXLoc > 0)
		{ toNW = cumalativeEnergyMap[i - 1][minXLoc - 1]; }
		
		if (minXLoc < width - 1)
		{ toNE = cumalativeEnergyMap[i - 1][minXLoc + 1]; }
		
		if (toNW <= toN)
		{ 
			locOffset = -1;
			lowest = toNW;
		}
		else
		{
			locOffset = 0;
			lowest = toN;
		}
		
		if (toNE < lowest)
		{ 
			locOffset = 1;
			lowest = toNE;
		}
		
		minXLoc += locOffset; 
		
		// Remove the newly found next cell of the seam from the image.
		imageMap[i - 1].erase(imageMap[i - 1].begin() + minXLoc); 
	}	

}	


void Transpose(const vector<vector<Pixel>> &map, vector<vector<Pixel>> &transposedMap, const short &width, const short &height)
{
	#pragma omp parallel for
	for (short i = 0; i < height; i++)
	{
        for (short j = 0; j < width; j++)
        {
            transposedMap[j][i] = map[i][j];
        }
    }
        
}


void InitEnergyMap(vector<vector<short>> &energyMap, const short &width, const short &height)
{
	energyMap.resize(height);
		
	for (auto &energyRow : energyMap)
    {
        for (short i = 0; i < width; i++)
        {
            energyRow.push_back(0);
        }
    }
    
}











