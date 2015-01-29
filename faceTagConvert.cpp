// faceTagConvert.cpp: Benjamin Henne

#include <exiv2/exiv2.hpp>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <vector>

std::string MWGRSDEFAULTTYPE = "Face";


struct mpr_tag {
    std::string personDisplayName;
    float x;  // top-left corner x
    float y;  // top-left corner y
    float w;  // region width
    float h;  // region height
    mpr_tag(){};
    mpr_tag(std::string name) {
        personDisplayName = name;
        x = y = w = h = 0;
    }
    mpr_tag(std::string name, float tx, float ty, float tw, float th) {
        personDisplayName = name;
        x = tx;
        y = ty;
        w = tw;
        h = th;
    }
};


int main(int argc, char* const argv[]) {
  try {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <image file>" << std::endl;
        return 1;
    }
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(argv[1]);
    assert(image.get() != 0);
    image->readMetadata();

    int imageWidth = image->pixelWidth();
    int imageHeight = image->pixelHeight();

    Exiv2::XmpData &xmpData = image->xmpData();
    if (xmpData.empty()) {
        std::string error(argv[1]);
        error += ": No XMP metadata found in file.";
        throw Exiv2::Error(1, error);
    }


    bool mpr = (xmpData.findKey(Exiv2::XmpKey("Xmp.MP.RegionInfo")) != xmpData.end());
    if (mpr) {
        std::cout << "MP" << std::endl;
    }
    bool mwgrs = (xmpData.findKey(Exiv2::XmpKey("Xmp.mwg-rs.Regions")) != xmpData.end());
    if (mwgrs) {
        std::cout << "MWG" << std::endl;
    }

    if (mpr && mwgrs) {
        std::cout << "Found Microsoft Photo Region Info and Metadata Working Group Regions Schema information.\n Just doing nothing";
        // How merging? If names differ ok, but if not
        return 1;
    }


    std::vector<mpr_tag> mpr_tags;
    if (mpr && !mwgrs) {
        int i = 1;
        Exiv2::XmpData::iterator it = xmpData.findKey(Exiv2::XmpKey("Xmp.MP.RegionInfo/MPRI:Regions"));
        if (it != xmpData.end()) {
            while (42) {
                std::string name = "";
                std::vector<float> rect;
                float x, y, w, h;
                try {
                    Exiv2::XmpData::iterator it = xmpData.findKey(Exiv2::XmpKey("Xmp.MP.RegionInfo/MPRI:Regions["+std::to_string(i)+"]/MPReg:PersonDisplayName"));
                    if (it == xmpData.end()) throw Exiv2::Error(1, "Key not found");
                    // should rather use getValue() and downcast? see http://www.exiv2.org/example2.html
                    std::string key = "Xmp.MP.RegionInfo/MPRI:Regions["+std::to_string(i)+"]/MPReg:PersonDisplayName";
                    Exiv2::Xmpdatum d = xmpData[key];
                    name = d.value().toString();
                } catch (Exiv2::Error& e) { break; }
                try {
                    // here the simple way is ok, since we do not bother to add unwanted data (??) -- or use findKey here either.
                    std::string r =  xmpData["Xmp.MP.RegionInfo/MPRI:Regions["+std::to_string(i)+"]/MPReg:Rectangle"].value().toString();
                    std::stringstream ss(r);
                    std::vector<float> v;
                    while (ss.good()) {
                        std::string substr;
                        getline(ss, substr, ',');
                        float f;
                        std::istringstream(substr)>>f;
                        v.push_back(f);
                    }
                    if (v.size() == 4) {
                        x = v[0];
                        y = v[1];
                        w = v[2];
                        h = v[3];
                    }
                } catch (Exiv2::Error& e) { }
                if (name.size() > 0) {
                    mpr_tags.emplace_back(name, x, y, w, h);
                }
                i++;
            }
        }
    }


    // make backup copy like Exiftool
    std::ifstream source(argv[1], std::ios::binary);
    std::string destfile = argv[1];
    destfile += "_original";
    std::ofstream dest(destfile, std::ios::binary);
    dest << source.rdbuf();
    source.close();
    dest.close();

    // write MWG-RS
    // if MP data und size>0
    xmpData["Xmp.mwg-rs.Regions/mwg-rs:AppliedToDimensions/stDim:w"] = imageWidth;
    xmpData["Xmp.mwg-rs.Regions/mwg-rs:AppliedToDimensions/stDim:h"] = imageHeight;
    xmpData["Xmp.mwg-rs.Regions/mwg-rs:AppliedToDimensions/stDim:unit"] = "pixel";
    xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList"] = "";

    int i = 1;
    for (std::vector<mpr_tag>::iterator it = mpr_tags.begin() ; it != mpr_tags.end(); ++it) {
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Name"] = it->personDisplayName;
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Type"] = MWGRSDEFAULTTYPE;
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Area/stArea:x"] = it->x + it->w/2;
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Area/stArea:y"] = it->y + it->h/2;
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Area/stArea:w"] = it->w; 
            // no adjustment needed, since appliedDim=imageWidth
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Area/stArea:h"] = it->h;
        xmpData["Xmp.mwg-rs.Regions/mwg-rs:RegionList["+std::to_string(i)+"]/mwg-rs:Area/stArea:unit"] = "normalized"; 
        i++;
    }


    // WRITE to file
    image->setXmpData(xmpData);
    std::cout << "XMARKER" << std::endl;
    image->writeMetadata();


    for (Exiv2::XmpData::const_iterator md = xmpData.begin(); 
         md != xmpData.end(); ++md) {
        std::cout << std::setfill(' ') << std::left
                  << std::setw(44)
                  << md->key() << " "
                  << std::setw(9) << std::setfill(' ') << std::left
                  << md->typeName() << " "
                  << std::dec << std::setw(3)
                  << std::setfill(' ') << std::right
                  << md->count() << "  "
                  << std::dec << md->value()
                  << std::endl;
    }

  
  }
  catch (Exiv2::Error& e) {
      std::cout << "Error: " << e.what() << std::endl;
      return -1;
  }
  catch (Exiv2::AnyError& e) {
      std::cout << e.what() << std::endl;
  }
}
