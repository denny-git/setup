
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-lib.h"
#include "decoder/decoder-wrappers.h"
#include "nnet3/nnet-am-decodable-simple.h"
#include "nnet3/nnet-utils.h"
#include "feat/feature-mfcc.h"
#include "feat/wave-reader.h"
#include "transform/cmvn.h"
#include "online2/online-ivector-feature.h"
#include "httplib.h"
#include <chrono>
#include <cstdio>
#include <codecvt>
#include <fstream>
#include <sstream>
#include <string>
#define NUMBERCODE 20

using namespace kaldi;
using namespace kaldi::nnet3;
using namespace std;
typedef kaldi::int32 int32;
using fst::SymbolTable;
using fst::Fst;
using fst::StdArc;

MfccOptions mfcc_opts;
OnlineIvectorExtractionConfig ivector_config;
NnetSimpleComputationOptions decodable_opts;
LatticeFasterDecoderConfig config;
TransitionModel trans_model;
AmNnetSimple am_nnet;
int transfer[NUMBERCODE];
CachingOptimizingCompiler compiler(am_nnet.GetNnet(), decodable_opts.optimize_config);
fst::SymbolTable* word_syms = NULL;
Fst<StdArc>* decode_fst = NULL;

const char* html = R"(
<form id="formElem" enctype="multipart/form-data">
  <input type="file" name="sound_file" accept=".wav">
  <input type="submit">
</form>
<h1>Text will appear in the space below</h1>
<div id="div"></div>
<script>
  formElem.onsubmit = async (e) => {
    e.preventDefault();
	document.getElementById("div").innerHTML = "Processing...";
    let res = await fetch('/wav2text', {
      method: 'POST',
      body: new FormData(formElem)
    });
	if (res.status === 200) {
		document.getElementById("div").innerHTML = await res.text();
	} else {
		document.getElementById("div").innerHTML = "<font color=\"red\">Error " + await res.status + ". " + await res.text() + "</font>";
	}
    console.log(await res.text());
  };
</script>
)";

const char* html2 = R"(
<form id="formElem" enctype="multipart/form-data">
  <input type="file" name="sound_file" accept=".pcm">
  <input type="submit">
</form>
<h1>Text will appear in the space below</h1>
<div id="div"></div>
<script>
  formElem.onsubmit = async (e) => {
    e.preventDefault();
	document.getElementById("div").innerHTML = "Processing...";
    let res = await fetch('/pcm2text', {
      method: 'POST',
      body: new FormData(formElem)
    });
	if (res.status === 200) {
		document.getElementById("div").innerHTML = await res.text();
	} else {
		document.getElementById("div").innerHTML = "<font color=\"red\">Error " + await res.status + ". " + await res.text() + "</font>";
	}
    console.log(await res.text());
  };
</script>
)";

#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"

using namespace httplib;

std::string dump_headers(const Headers& headers) {
	std::string s;
	char buf[BUFSIZ];

	for (auto it = headers.begin(); it != headers.end(); ++it) {
		const auto& x = *it;
		snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
		s += buf;
	}

	return s;
}

std::string log(const Request& req, const Response& res) {
	std::string s;
	char buf[BUFSIZ];

	s += "================================\n";

	snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
		req.version.c_str(), req.path.c_str());
	s += buf;

	std::string query;
	for (auto it = req.params.begin(); it != req.params.end(); ++it) {
		const auto& x = *it;
		snprintf(buf, sizeof(buf), "%c%s=%s",
			(it == req.params.begin()) ? '?' : '&', x.first.c_str(),
			x.second.c_str());
		query += buf;
	}
	snprintf(buf, sizeof(buf), "%s\n", query.c_str());
	s += buf;

	s += dump_headers(req.headers);

	s += "--------------------------------\n";

	snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
	s += buf;
	s += dump_headers(res.headers);
	s += "\n";

	if (!res.body.empty()) { s += res.body; }

	s += "\n";

	return s;
}


typedef struct  WAV_HEADER {
	char                RIFF[4];        // RIFF Header      Magic header
	unsigned int        ChunkSize;      // RIFF Chunk Size  
	char                WAVE[4];        // WAVE Header      
	char                fmt[4];         // FMT header       
	unsigned int        Subchunk1Size;  // Size of the fmt chunk                                
	unsigned short      AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
	unsigned short      NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
	unsigned int        SamplesPerSec;  // Sampling Frequency in Hz                             
	unsigned int        bytesPerSec;    // bytes per second 
	unsigned short      blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
	unsigned short      bitsPerSample;  // Number of bits per sample      
	char                Subchunk2ID[4]; // "data"  string   
	unsigned int        Subchunk2Size;  // Sampled data length    
} wavhdr;

namespace kaldi {

	namespace nnet3 {

		std::wstring wave2text(float *wlData, int L,
			MfccOptions mfcc_opts,
			OnlineIvectorExtractionConfig ivector_config,
			NnetSimpleComputationOptions decodable_opts,
			TransitionModel *trans_model,
			AmNnetSimple am_nnet,
			CachingOptimizingCompiler *compiler,
			fst::SymbolTable *word_syms,
			int *transfer,
			LatticeFasterDecoder &decoder);


		int loadmodel(
			MfccOptions *mfcc_opts,
			OnlineIvectorExtractionConfig *ivector_config,
			NnetSimpleComputationOptions *decodable_opts,
			LatticeFasterDecoderConfig *config,
			TransitionModel *trans_model,
			int *transfer,
			AmNnetSimple *am_nnet);

	}
}

int loadEverything() {
	int count = loadmodel(&mfcc_opts, &ivector_config, &decodable_opts, &config, &trans_model, transfer, &am_nnet);
	

	std::wcout << " loading HCLG ... " << std::endl << std::endl;

	decode_fst = fst::ReadFstKaldiGeneric("model\\HCLG.fst");

	std::wcout << " loading Word Symbols ... " << std::endl << std::endl;

	word_syms = fst::SymbolTable::ReadText1("model\\words.txt");

	std::wcout << " Word Symbols has been loaded. " << std::endl << std::endl;
	return count;
	//  ==============================================
}
std::wstring text(FILE *wav) {

	LatticeFasterDecoder decoder(*decode_fst, config);

	wavhdr wavHeader;
	unsigned int resultR;
	short* wData;
	float* wlData;
	int L;
	//read wave header
	fread(&wavHeader, sizeof(wavhdr), 1, wav);

	//determine size of buffer
	L = (int)wavHeader.Subchunk2Size * sizeof(short) / sizeof(wavHeader.ChunkSize);
	cout << std::endl;
	cout << L << std::endl;
	cout << sizeof(*wav) << std::endl;
	cout << "Chunk size: " << wavHeader.ChunkSize << std::endl;
	cout << "Subchunk2 size: "<< wavHeader.Subchunk2Size << std::endl;

	//short datatype buffer
	wData = (short*)malloc(sizeof(short) * L);

	//float datatype buffer
	wlData = (float*)malloc(sizeof(float) * L);

	//read file to wData buffer
	fread(wData, sizeof(short), L, wav);

	fclose(wav);

	//convert from short to float
	for (int i = 0; i < L; i++)
		wlData[i] = (float)wData[i];

	std::wstring text1 = L" speech waveform input: ";
	std::wcout << std::endl << std::endl;
	std::wcout << text1 << "(j1 + 1)" << " : length = " << L << " : " << std::endl << std::endl;

	std::wstring text;
	text = wave2text(wlData, L, mfcc_opts, ivector_config, decodable_opts, &trans_model, am_nnet, &compiler, word_syms, transfer, decoder);

	std::wcout << text << std::endl << std::endl;
	return text;
}
std::wstring pcmtext(FILE* wav, const int fileSize) {

	LatticeFasterDecoder decoder(*decode_fst, config);
	unsigned int resultR;
	short* wData;
	float* wlData;
	int L;
	//read wave header
	//fread(&wavHeader, sizeof(wavhdr), 1, wav);

	//determine size of buffer
	const int ChunkSize = fileSize - 8;
	const int Subchunk2Size = fileSize - 44;
	L = (int)Subchunk2Size * sizeof(short) / sizeof(ChunkSize);
	cout << std::endl;
	cout << L << std::endl;
	//short datatype buffer
	wData = (short*)malloc(sizeof(short) * L);

	//float datatype buffer
	wlData = (float*)malloc(sizeof(float) * L);

	//read file to wData buffer
	fread(wData, sizeof(short), L, wav);

	fclose(wav);

	//convert from short to float
	for (int i = 0; i < L; i++)
		wlData[i] = (float)wData[i];

	std::wstring text1 = L" speech waveform input: ";
	std::wcout << std::endl << std::endl;
	std::wcout << text1 << "(j1 + 1)" << " : length = " << L << " : " << std::endl << std::endl;

	std::wstring text;
	text = wave2text(wlData, L, mfcc_opts, ivector_config, decodable_opts, &trans_model, am_nnet, &compiler, word_syms, transfer, decoder);

	std::wcout << text << std::endl << std::endl;
	return text;
}
std::wstring openFilesTemp(const string filePath) {
	FILE* wavFile;
	// --- AUDIO WAVE ---
	//open file
	wavFile = fopen(filePath.c_str(), "rb");

	if (wavFile == NULL) { printf("unable to open wave file\n"); }

	std::wstring textOutput = text(wavFile);
	fclose(wavFile);
	std::wcout << filePath.c_str() << std::endl;
	return textOutput;
}
std::wstring openPCMFilesTemp(const string filePath, int fileSize) {
	FILE* wavFile;

	// --- AUDIO WAVE ---
	//open file
	wavFile = fopen(filePath.c_str(), "rb");

	if (wavFile == NULL) { printf("unable to open pcm file\n"); }

	std::wstring textOutput = pcmtext(wavFile, fileSize);
	fclose(wavFile);
	std::wcout << filePath.c_str() << std::endl;
	return textOutput;
}
int main(int argc, char *argv[]) 
{
		Server svr;

		int count = loadEverything();

		svr.Get("/file", [](const Request&, Response& res) {
			res.set_content(html, "text/html");
		});

		svr.Get("/pcmfile", [](const Request&, Response& res) {
			res.set_content(html2, "text/html");
		});
			

		svr.Post("/wav2text", [](const Request& req, Response& res) {
			auto image_file = req.get_file_value("sound_file"); //get uploaded file from request
			std::wstring text;

			cout << "image file length: " << image_file.content.length() << endl
				<< "image file name: " << image_file.filename << image_file.content_type << endl;

			/*if (image_file.content_type != "audio/wav" && image_file.content_type != "audio/wave" && image_file.content_type != "audio/x-wav") {
				res.set_content("File type must be .wav.", "text/plain");
				res.status = 400;
			}
			else if (image_file.content.length() > 3200000) {
				res.set_content("File size is too big.", "text/plain");
				res.status = 400;
			}
			else if (image_file.content.length() == 0) {
				res.set_content("Received an empty file.", "text/plain");
				res.status = 400;
			}*/
			if (false) {

			}
			else {
				{
					//ofstream ofs(image_file.filename, ios::binary);
					//ofs << image_file.content;
					ofstream imageFile;
					std::string name1 = std::tmpnam(nullptr); //generate random file name in the temp directory

					std::cout << name1;

					name1 += ".wav"; //indicate that the file is a wav file

					imageFile.open(name1, std::ios::binary); //call openFiles, then delete

					imageFile << image_file.content;
					imageFile.close();
					text = openFilesTemp(name1);
					remove(name1.c_str()); //delete the audio file once converted to text

					std::wcout << text;

					//convert output to string for cpp-httplib to return in response
					std::string convertedText = std::string(text.begin(), text.end()); 

					res.set_content(convertedText, "text/plain");
				}
			}
			});
		svr.Post("/pcm2text", [](const Request& req, Response& res) {
			auto image_file = req.get_file_value("sound_file");
			std::wstring text;

			cout << "image file length: " << image_file.content.length() << endl
				<< "image file name: " << image_file.filename << image_file.content_type << endl;
			/*
			if (image_file.content_type != "audio/wav" && image_file.content_type != "audio/wave" && image_file.content_type != "audio/x-wav") {
				res.set_content("File type must be .wav.", "text/plain");
				res.status = 400;
			}*/
			/*else*/ 
			/*if (image_file.content.length() > 3200000) {
				res.set_content("File size is too big.", "text/plain");
				res.status = 400;
			}
			else*/ if (image_file.content.length() == 0) {
				res.set_content("Received an empty file.", "text/plain");
				res.status = 400;
			}
			else {
				{
					//ofstream ofs(image_file.filename, ios::binary);
					//ofs << image_file.content;
					ofstream imageFile;

					std::string name1 = std::tmpnam(nullptr); //generate random file name in the temp directory
					std::cout << name1;

					name1 += ".pcm";  //indicate that the file is a wav file
					imageFile.open(name1, std::ios::binary);
					imageFile << image_file.content;
					imageFile.close();
					text = openPCMFilesTemp(name1, image_file.content.length());

					remove(name1.c_str()); //delete audio file once converted to text
					std::wcout << text;

					//convert output to string for cpp-httplib to return in response
					std::string convertedText = std::string(text.begin(), text.end());
					res.set_content(convertedText, "text/plain");
				}
			}
			});
		svr.Post("/testing", [](const Request& req, Response& res) {
			res.set_content("Post test.", "text/plain");

			});
		svr.set_logger([](const Request& req, const Response& res) {
			printf("%s", log(req, res).c_str());
			});
		std::wcout << "Ready to convert speech to text! Server is listening on localhost:8080" << std::endl;
		svr.listen("localhost", 8080);
		return 0;
}
