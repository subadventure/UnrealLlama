
#include "LlamaComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"


// Sets default values for this component's properties
ULlamaComponent::ULlamaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	llama_backend_init();
}

void ULlamaComponent::BeginPlay()
{
	Super::BeginPlay();

}
void ULlamaComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	CancelDownload = true;

	if (!IsGenerating)UnloadModel();
	else IsCancelled = true;
	IsPaused = false;
	
}

const bool ULlamaComponent::IsInitialized()
{
	if (!LlamaModel) {
		UKismetSystemLibrary::PrintString(GetWorld(), "No Model found.. loading Model");

		const FString Path = FPaths::Combine(FPaths::GameSourceDir(), "ThirdParty/Models/", ModelName);

		LlamaModel = llama_model_load_from_file(TCHAR_TO_ANSI(*Path), llama_model_default_params());

		if (!LlamaModel) {
			UKismetSystemLibrary::PrintString(GetWorld(), "Could not initialize Model...");
			return false;
		}
		else PrintString("Model successfully initialized!");

		if (!LlamaContext) {
			llama_context_params cparams = llama_context_default_params();
			cparams.n_ctx = 2048;   // Kontextgröße = Gedächtnis
			cparams.n_threads = 8;

			LlamaContext = llama_new_context_with_model(LlamaModel, cparams);

			if (!LlamaContext) {
				UKismetSystemLibrary::PrintString(GetWorld(), "Could not initialize Context...");
				return false;
			}
			else UKismetSystemLibrary::PrintString(GetWorld(), "Context initialized successfully!...");

			if (!LlamaSampler) {
				InitSampler(FSamplerParams());

				if (!LlamaSampler || !LlamaCommonSampler) {
					UKismetSystemLibrary::PrintString(GetWorld(), "Could not initialize Sampler..");
					return false;
				}
				else UKismetSystemLibrary::PrintString(GetWorld(), "Sampler initialized successfully!");
			}
		}
	}
	return LlamaModel != nullptr && LlamaContext != nullptr && LlamaSampler != nullptr;
}

void ULlamaComponent::InitSampler(FSamplerParams InParams)
{
	UKismetSystemLibrary::PrintString(GetWorld(), "No Sampler found.. initialising Sampler");

	common_params_sampling params = InParams.GetParams();

	LlamaCommonSampler = common_sampler_init(LlamaModel, params);

	llama_sampler_chain_params cparams = llama_sampler_chain_default_params();
	LlamaSampler = llama_sampler_chain_init(cparams);

	// 2) Module hinzufügen (Reihenfolge ist relevant)
	llama_sampler_chain_add(LlamaSampler, llama_sampler_init_temp(params.temp));
	llama_sampler_chain_add(LlamaSampler, llama_sampler_init_top_k(params.top_k));
	llama_sampler_chain_add(LlamaSampler, llama_sampler_init_top_p(params.top_p, /*min_keep*/1));
	llama_sampler_chain_add(LlamaSampler, llama_sampler_init_min_p(params.min_p, /*min_keep*/1));

	// Optional: Mirostat als Selector, sonst simpler RNG-Selector
	if (params.mirostat) {
		// Beispielwerte: target entropy 5.0, eta 0.1 (tune nach Geschmack)
		llama_sampler_chain_add(LlamaSampler, llama_sampler_init_mirostat_v2(/*target*/5.0f, /*eta*/0.1f, /*m*/100));
	}
	else {
		llama_sampler_chain_add(LlamaSampler, llama_sampler_init_dist(params.seed));
	}
}
void ULlamaComponent::InitGenerating(const FString& InInstructions, const FString& InText)
{
	if (IsInitialized()) {

		ResponseText = "";

		llama_tokens tokens = common_tokenize(LlamaContext, TCHAR_TO_ANSI(*RolePrompt(InInstructions, InText)), true);

		Batch = llama_batch_init(512, 0, 1);

		for (auto id : tokens) {
			//PrintString("token: " + FString(common_token_to_piece(LlamaContext, id).c_str()));
		}

		common_batch_clear(Batch);

		for (int i = 0; i < tokens.size(); i++) {
			common_batch_add(Batch, tokens[i], i, { 0 }, false);
		}

		Batch.logits[Batch.n_tokens - 1] = true;
		llama_decode(LlamaContext, Batch);

		IsGenerating = true;
		CurrentIndex = Batch.n_tokens;

		AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [this]() {
			while (!IsCancelled && !IsStopped) {
				if (IsPaused)continue;
				/* Work on a dedicated thread */
				//const int new_token_id = llama_sampler_sample(Component->LlamaSampler, LlamaContext, -1);
				const int new_token_id = common_sampler_sample(LlamaCommonSampler, LlamaContext, -1);

				if (llama_vocab_is_eog(llama_model_get_vocab(LlamaModel), new_token_id)) {
					break;
				}

				// --- piece extraction (sicher) ---
				//std::vector<char> buf(512);
				//const int num = llama_token_to_piece(llama_model_get_vocab(LlamaModel),new_token_id, buf.data(), (int)buf.size(), 0, true);

				std::string Piece = common_token_to_piece(LlamaContext, new_token_id);

				if (Piece != "") {
					const FString Clean = FilterPrintable(FString(UTF8_TO_TCHAR(Piece.c_str())));

					if (!Clean.IsEmpty() && !IsCancelled) {

						ResponseText += Clean;
						AsyncTask(ENamedThreads::GameThread, [this, Text = ResponseText]() {if (IsValid(UserInterface))UserInterface->TextChanged(Text); });
					}
				}

				// next step
				common_batch_clear(Batch);
				common_batch_add(Batch, new_token_id, CurrentIndex++, { 0 }, true);

				if (llama_decode(LlamaContext, Batch) < 0) {
					break;
				}
			}

			llama_sampler_accept(LlamaSampler, llama_vocab_eot(llama_model_get_vocab(LlamaModel)));
			llama_kv_self_clear(LlamaContext);

			if (IsCancelled)UnloadModel();
			else IsGenerating = false; IsStopped = false; 

		});
	}
}

void ULlamaComponent::StartGenerating()
{
	IsPaused = false;
}
void ULlamaComponent::PauseGenerating()
{
	IsPaused = true;
}
void ULlamaComponent::StopGenerating()
{
	IsStopped = true;
	IsPaused = false;
}
void ULlamaComponent::UnloadModel()
{
	if (LlamaContext) {
		llama_free(LlamaContext);
		LlamaContext = nullptr;
	}
	if (LlamaModel) {
		llama_model_free(LlamaModel);
		LlamaModel = nullptr;
	}
	llama_backend_free(); // 🟢 Backend immer zuletzt freigeben
}

// HELPER FUNCTIONS
TArray<FString> ULlamaComponent::GetModelPaths()
{
	PrintString("\n\nGetModelPaths\n\n");

	const FString Path = FPaths::Combine(FPaths::GameSourceDir(), "ThirdParty/Models");
	
	TArray<FString> FileNames;

	// Alle Dateien (Wildcard = "*.*")
	IFileManager& FileManager = IFileManager::Get();
	FileManager.IterateDirectory(*Path,
		[&FileNames](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
		{
			if (!bIsDirectory) // nur Dateien, keine Ordner
			{
				FileNames.Add(FString(FilenameOrDirectory));
			}
			return true; // true = weiter iterieren
		});

	// → Achtung: Das gibt nur Dateinamen zurück, ohne den kompletten Pfad.
	// Wenn du vollständige Pfade willst, musst du sie zusammensetzen:
	for (FString& File : FileNames)
	{
		FString Left;
		FString Right;
		File.Split("/", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		File = Right;
		PrintString(File);
	}

	return FileNames;
}

const FString ULlamaComponent::RolePrompt(const FString& InInstructions, const FString& InText) {
	FString Prompt =
		"<|im_start|>system\n"
		+ InInstructions + "<|im_end|>\n"
		"<|im_start|>user\n"
		+ InText + "<|im_end|>\n"
		"<|im_start|>assistant\n";
	return Prompt;
}
const bool ULlamaComponent::is_valid_utf8(const char* string) {
	if (!string) {
		return true;
	}

	const unsigned char* bytes = (const unsigned char*)string;
	int num;

	while (*bytes != 0x00) {
		if ((*bytes & 0x80) == 0x00) {
			// U+0000 to U+007F
			num = 1;
		}
		else if ((*bytes & 0xE0) == 0xC0) {
			// U+0080 to U+07FF
			num = 2;
		}
		else if ((*bytes & 0xF0) == 0xE0) {
			// U+0800 to U+FFFF
			num = 3;
		}
		else if ((*bytes & 0xF8) == 0xF0) {
			// U+10000 to U+10FFFF
			num = 4;
		}
		else {
			return false;
		}

		bytes += 1;
		for (int i = 1; i < num; ++i) {
			if ((*bytes & 0xC0) != 0x80) {
				return false;
			}
			bytes += 1;
		}
	}

	return true;
}
const FString ULlamaComponent::FilterPrintable(const FString& In)
{
	FString Out;
	for (int32 i = 0; i < In.Len(); ++i) {
		TCHAR c = In[i];
		if (c >= 0x20 && c <= 0xD7FF || (c >= 0xE000 && c <= 0x10FFFF))
			Out.AppendChar(c);
	}
	return Out;
}
void ULlamaComponent::DownloadGGUFFile(const FString& Url, const FString& FileName)
{
	PrintString("DownloadGGUFFile");
	// Speicherpfad vorbereiten

	const FString FinalPath = FPaths::Combine(FPaths::GameSourceDir(), "ThirdParty/Models/", FileName);

	TSharedRef<FArchive, ESPMode::ThreadSafe> Archive =MakeShareable(IFileManager::Get().CreateFileWriter(*FinalPath));

	if (!Archive->IsSaving())
	{
		UE_LOG(LogTemp, Error, TEXT("Konnte Datei %s nicht anlegen!"), *FinalPath);
		return;
	}

	// HTTP Request erstellen
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->SetURL(Url);

	// Dem Request sagen, dass die Response direkt ins FileArchive geschrieben werden soll
	HttpRequest->SetResponseBodyReceiveStream(Archive);
	HttpRequest->OnHeaderReceived().BindLambda([this](FHttpRequestPtr Request, const FString& HeaderName, const FString& NewHeaderValue)
		{
			if (HeaderName.Equals(TEXT("Content-Length"), ESearchCase::IgnoreCase))
			{
				ContentLength = UKismetStringLibrary::Conv_StringToInt64(NewHeaderValue);
			}
		});
	// Fortschrittsanzeige
	HttpRequest->OnRequestProgress64().BindLambda([this, Archive, FinalPath](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
		{
			if (CancelDownload) {
				Request->CancelRequest();
				Archive->Close();
				IFileManager::Get().Delete(*FinalPath);
			}

			/*
			PrintString(
				"Progress:" + FString::SanitizeFloat(Progress) + 
				" BytesSend:" + UKismetStringLibrary::Conv_Int64ToString(BytesSent) + 
				" BytesReceived:" + UKismetStringLibrary::Conv_Int64ToString(BytesReceived) +
				" ContentLength:" + UKismetStringLibrary::Conv_Int64ToString(ContentLength)
			);
			*/

			const int64 Recieved = BytesReceived > 0 ? BytesReceived : 4294967294 + BytesReceived;
			const float Progress = float(Recieved) / float(ContentLength) * 100.0;

			AsyncTask(ENamedThreads::GameThread, [this, Progress]() {
				if (!CancelDownload && IsValid(UserInterface))UserInterface->DownloadProgress(Progress);
			});
		});

	// Abschluss
	HttpRequest->OnProcessRequestComplete().BindLambda([this, Archive, FinalPath](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			// Archive schließen
			Archive->Close();

			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				IFileManager::Get().Delete(*FinalPath);
				UE_LOG(LogTemp, Error, TEXT("Download fehlgeschlagen!"));
				return;
			}

			if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
			{
				UE_LOG(LogTemp, Log, TEXT("Download erfolgreich gespeichert unter %s"), *FinalPath);
			}
			else
			{
				IFileManager::Get().Delete(*FinalPath);
				UE_LOG(LogTemp, Error, TEXT("Download fehlgeschlagen mit Code %d"), Response->GetResponseCode());
			}
		});

	HttpRequest->ProcessRequest();
}
