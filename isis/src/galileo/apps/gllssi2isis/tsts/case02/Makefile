APPNAME = gllssi2isis

labels.txt.IGNORELINES = Bytes StartByte ByteOrder TileSamples TileLines

include $(ISISROOT)/make/isismake.tsts

commands:
	 $(APPNAME) from=$(INPUT)/1213r.lbl \
	  to=$(OUTPUT)/gllssi2isisTruth2.cub > /dev/null;
	  catlab from=$(OUTPUT)/gllssi2isisTruth2.cub to=$(OUTPUT)/labels.txt > /dev/null;
