#include "SCLDump.h"
#include "iostream"

auto printinst = [](const SCLInstructionData& data) {
	int mx = data.cnt - 1;
	printf("%s ", g_InstructionSize[data.cmd].name);
	for (int i = 0; i <= mx; i++) {
		SCL_DATATYPE datatype = data.param[i].datatype;
		switch (datatype) {
		case ADDRESS:
			if (data.cmd >= SCR_TJMP && data.cmd <= SCR_LJMP) {
				address ads = data.param[i].ads;
				if (g_LabelData.find(ads) != g_LabelData.end())
					printf("%s", g_LabelData[ads].name.c_str());
			}
			else {
				address ads = data.param[i].ads;
				if (g_ProcData.find(ads) != g_ProcData.end())
					printf("%s", g_ProcData[ads].name.c_str());
				else 
					printf("0x%x", data.param[i].ads);
			}
			break;
		case U32:
			printf("%d", data.param[i].sdword);
			break;
		case I32:
			printf("%d", data.param[i].dword);
			break;
		case U16:
			printf("%d", data.param[i].word);
			break;
		case I16:
			printf("%d", data.param[i].sword);
			break;
		case COMMAND:
			break;
		case U8:
			printf("%d", data.param[i].byte);
			break;
		case I8:
			printf("%d", data.param[i].sbyte);
			break;
		case STRING:
			printf("\"%s\"", data.param[i].stringdata);
			break;
		}
		if (i != mx && datatype != COMMAND) {
			if (!((data.cmd == SCR_PUSHR || data.cmd == SCR_POPR || data.cmd == SCR_PARENT) && i == 0)) {
				printf(", ");
			}	
		}
	}
};

bool DumpSCL(const char* Name) {
	FILE* fp = nullptr;
	size_t size = 0;
	void* data = nullptr;

	//Open file and see if exists
	if (fp = fopen(Name, "rb")) {
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		rewind(fp);
		data = malloc(size + 0x200);
		fread(data, size, 1, fp);
		fclose(fp);

		//Now, read all data
		ReadSCL(data, size, Name);

		//Finally, free memory
		free(data);
		return true;
	}
	//nothing happened here
	return false;
}

void ReadSCLHeader(SCLHeader* pHeader) {
	g_ProcData.insert({ pHeader->TexInitializer , {PROC_TEXINIT, "TEXINIT"} });
	for (int i = 0; i < pHeader->NumLv1SCL; i++) {
		g_ProcData.insert({ pHeader->SCL_Lv1[i] , {PROC_SET, "LVL1PROC_" + std::to_string(i)} });
	}
	for (int i = 0; i < pHeader->NumLv2SCL; i++) {
		g_ProcData.insert({ pHeader->SCL_Lv2[i] , {PROC_SET, "LVL2PROC_" + std::to_string(i)} });
	}
	for (int i = 0; i < pHeader->NumLv3SCL; i++) {
		g_ProcData.insert({ pHeader->SCL_Lv3[i] , {PROC_SET, "LVL3PROC_" + std::to_string(i)} });
	}
	for (int i = 0; i < pHeader->NumLv4SCL; i++) {
		g_ProcData.insert({ pHeader->SCL_Lv4[i] , {PROC_SET, "LVL4PROC_" + std::to_string(i) } });
	}

	for (int i = 0; i < 9; i++) {
		const std::string& charname = ID2String(i);
		g_ProcData.insert({ pHeader->BossAddr[i],	{PROC_ENEMY ,charname + "_BOSS"	} });
		g_ProcData.insert({ pHeader->ComboAddr[i],	{PROC_ENEMY ,charname + "_COMBO"} });
		g_ProcData.insert({ pHeader->Lv1Attack[i],	{PROC_ENEMY ,charname + "_ATK1"} });
		g_ProcData.insert({ pHeader->Lv2Attack[i],	{PROC_ENEMY ,charname + "_ATK2"} });
		g_ProcData.insert({ pHeader->ExAnmLv1[i],	{PROC_EXANM ,charname + "_ANM1"} });
		g_ProcData.insert({ pHeader->ExAnmLv2[i],	{PROC_EXANM ,charname + "_ANM2"} });
		g_ProcData.insert({ pHeader->ExAnmBoss[i],	{PROC_EXANM ,charname + "_BOSSANM"} });
		g_ProcData.insert({ pHeader->ExAnmWin[i],	{PROC_EXANM ,charname + "_WINANM"} });
		size_t num_tex = pHeader->LTEntry[i].NumTextures;
		address* pTexEntry = pHeader->LTEntry[i].EntryPoint;
		for (int j = 0; j < num_tex; j++)
			if (pTexEntry[j] != 0xffffffff)
				g_ProcData.insert({ pTexEntry[j], { PROC_LOADTEX, charname + "_TEXENTRY" + std::to_string(j) } });
			else pHeader->LTEntry[i].NumTextures--;
	}

}

void PrintSCLHeader(SCLHeader* pHeader, const char* name) {
	std::ofstream off_file;
	off_file.open(std::string(name) + ".json", std::ios::out);
	off_file << "{\n" <<
	"	\"TexInit\" : \"TEXINIT\",\n" <<
	"	\"SCL1\" : [";
	for (int i = 0; i <= pHeader->NumLv1SCL - 1; i++) {
		off_file << "\"LVL1PROC_" << std::to_string(i)  << "\"";
		if (i < pHeader->NumLv1SCL - 1) off_file << ", ";
	}
	off_file << "],\n" <<
	"	\"SCL2\" : [";
	for (int i = 0; i <= pHeader->NumLv2SCL - 1; i++) {
		off_file << "\"LVL2PROC_" << std::to_string(i) << "\"";
		if (i < pHeader->NumLv2SCL - 1) off_file << ", ";
	}
	off_file << "],\n" <<
	"	\"SCL3\" : [";
	for (int i = 0; i <= pHeader->NumLv3SCL - 1; i++) {
		off_file << "\"LVL3PROC_" << std::to_string(i) << "\"";
		if (i < pHeader->NumLv3SCL - 1) off_file << ", ";
	}
	off_file << "],\n" <<
	"	\"SCL4\" : [";
	for (int i = 0; i <= pHeader->NumLv4SCL - 1; i++) {
		off_file << "\"LVL4PROC_" << std::to_string(i) << "\"";
		if (i < pHeader->NumLv4SCL - 1) off_file << ", ";
	}
	off_file << "],\n";


	for (int i = 0; i <= 8; i++) {
		const std::string& charname = ID2String(i);
		off_file << "	\"" << charname << "\" : {\n" <<
			"		\"Boss\" : \"" << charname << "_BOSS\",\n" <<
			"		\"Combo\" : \"" << charname + "_COMBO\",\n" <<
			"		\"Atk1\" : \"" << charname + "_ATK1\",\n" <<
			"		\"Atk2\" : \"" << charname + "_ATK2\",\n" <<
			"		\"Anm1\" : \"" << charname + "_ANM1\",\n" <<
			"		\"Anm2\" : \"" << charname + "_ANM2\",\n" <<
			"		\"BossAnm\" : \"" << charname + "_BOSSANM\",\n" <<
			"		\"WinAnm\" : \"" << charname + "_WINANM\",\n";
		size_t num_tex = pHeader->LTEntry[i].NumTextures;
		address* pTexEntry = pHeader->LTEntry[i].EntryPoint;
		off_file << "		\"LoadTex\" : [";
		for (int j = 0; j <= num_tex - 1; j++) {
			if (pTexEntry[j] != 0xffffffff) {
				off_file << "\"" << charname << "_TEXENTRY" << std::to_string(j) << "\"";
				if (j < num_tex - 1) off_file << ", ";
			}
		}
		if (i < 8) {
			off_file << "]\n" <<
				"	},\n";
		}
		else {
			off_file << "]\n" <<
				"	}\n";
		}
	}

	off_file << "}\n";
	off_file.close();
}

void ReadSCL(void* pData, size_t size, const char* name) {
	void* pHeader = pData;
	char* pSCLData = (char*)pData + sizeof(SCLHeader);
	size_t idx = 0 + sizeof(SCLHeader);
	u8 cmd = 0xff;
	SCLHeader header_data;
	memcpy(&header_data, pData, sizeof(SCLHeader));
	auto processData = [&](SCL_INSTRUCTION inst) {
		SCLInstructionDefine def = g_InstructionSize[inst];
		SCLInstructionData data;
		data.cnt = def.cnt;
		data.add = idx;
		int offset = 0;
		for (int i = 0; i < def.cnt; i++) {

			SCLParamData paramdata;
			paramdata.datatype = def.paramdatatype[i];
			switch (paramdata.datatype) {
			case ADDRESS: {
				address ads = *(address*)pSCLData;
				bool is_address = false;
				paramdata.ads = ads;
				const std::string& address = std::to_string(ads);
				ProcData pdata;
				switch (data.cmd) {
				case SCR_SET:
				case SCR_ESET:
				case SCR_CHILD:
				case SCR_CHGTASK:
					pdata.type = PROC_ENEMY;
					break;
				case SCR_CALL:
					//if (g_AddressData.find(ads) != g_AddressData.end()) {
					//	pdata.type = g_AddressData[ads].type;
					//}
					break;
				case SCR_ATK:
				case SCR_FATK:
				case SCR_ATKNP:
				case SCR_ATK2:
					pdata.type = PROC_ATK;
					break;
				case SCR_TASK:
					pdata.type = PROC_EXANM;
					break;

				case SCR_TJMP:
				case SCR_FJMP:
				case SCR_JMP:
				case SCR_OJMP:
				case SCR_AJMP:
				case SCR_LJMP:
					is_address = true;
					pdata.type = PROC_LABEL;
					break;
				}
				pdata.name = std::string("ADDRESS") + std::to_string(ads);
				
				if(is_address)g_LabelData.insert({ ads, std::move(pdata) });
				else g_ProcData.insert({ads, std::move(pdata)});
				pSCLData += 4;
				offset += 4;
			}	break;
			case U32:
				paramdata.sdword = *(u32*)pSCLData;
				pSCLData += 4;
				offset += 4;
				break;
			case I32:
				paramdata.dword = *(i32*)pSCLData;
				pSCLData += 4;
				offset += 4;
				break;
			case U16:
				paramdata.word = *(u16*)pSCLData;
				pSCLData += 2;
				offset += 2;
				break;
			case I16:
				paramdata.sword = *(i16*)pSCLData;
				pSCLData+=2;
				offset+=2;
				break;
			case COMMAND:
				data.cmd = *(SCL_INSTRUCTION*)pSCLData;
				pSCLData++;
				offset++;
				break;
			case U8:
				paramdata.byte = *(u8*)pSCLData;
				pSCLData++;
				offset++;
				break;
			case I8:
				paramdata.sbyte = *(i8*)pSCLData;
				pSCLData++;
				offset++;
				break;
			case STRING:
				paramdata.stringdata = pSCLData;
				while (*pSCLData++) offset++;
				offset++;
				break;
			}
			data.param.emplace_back(paramdata);
		}
		if (inst == SCR_ANIME) {
			u8 off = pSCLData[-1];
			for (int j = 0; j < off; j++) {
				SCLParamData paramdata;
				paramdata.datatype = U8;
				paramdata.byte = *(u8*)pSCLData;
				data.param.emplace_back(paramdata);
				data.cnt++;
				pSCLData++;
				offset++;
			}
		}
		idx += offset;
		return data;
	};

	ReadSCLHeader(&header_data);

	while (idx < size) {
		cmd = *pSCLData;
		//If less than this, is a command that uses registers
		if (cmd < SCR_NOP) cmd = *(pSCLData + 1);
		SCL_INSTRUCTION inst = static_cast<SCL_INSTRUCTION>(cmd);
		g_InstructionData.emplace_back(processData(inst));
	}
	PrintSCLHeader(&header_data, name);
	PrintSCL();
}

void PrintSCL() {
	address_map::iterator it = g_ProcData.begin();
	address_map::iterator it2 = g_LabelData.begin();
	int proc_cnt = 0;
	for (auto& i : g_InstructionData) {
		if (it != g_ProcData.end() && it->first <= i.add) {
			std::string ref = "";
			switch (it->second.type) {
			case PROC_TEXINIT: ref = "\nTEXINITPROC "; break;
			case PROC_ENEMY: ref = "\nENEMY "; break;
			case PROC_ATK: ref = "\nTSET "; break;
			case PROC_EXANM: ref = "\nEXANM "; break;
			case PROC_SET: ref = "\nSETPROC "; break;
			case PROC_LOADTEX: ref = "\nLOADTEXPROC "; break;
			default: ref = "\nPROC "; break;
			}
			if (proc_cnt > 0)
				printf("ENDPROC\n");
			std::cout << ref << it->second.name << "\n";
			it++;
			proc_cnt++;
		}
		if (it2 != g_LabelData.end() && it2->first <= i.add) {
			std::cout << it2->second.name << ":\n";
			it2++;
		}
		printf("\t"); printinst(i); printf("\n");
	}
	printf("ENDPROC\n");
}
