// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ThirdParty/llama.cpp/include/llama.h"
#include "ThirdParty/llama.cpp/ggml/include/ggml.h"
#include "ThirdParty/llama.cpp/common/sampling.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/ActorComponent.h"
#include "UserInterface.h"
#include "LlamaComponent.generated.h"

static llama_model* LlamaModel;
static llama_context* LlamaContext;

USTRUCT(BlueprintType)
struct FSamplerParams {
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category=llama)
	int Seed = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = llama)
	float Temp = 1.0;
	
	UPROPERTY(BlueprintReadWrite, Category = llama)
	int TopK = 40;
	
	UPROPERTY(BlueprintReadWrite, Category = llama)
	float TopP = 1.0;
	
	UPROPERTY(BlueprintReadWrite, Category = llama)
	float MinP = .05;
	
	UPROPERTY(BlueprintReadWrite, Category = llama)
	int Miro = 0;

	common_params_sampling GetParams() {
		common_params_sampling Params;
		Params.seed = Seed;
		Params.temp = Temp;
		Params.top_k = TopK;
		Params.top_p = TopP;
		Params.min_p = MinP;
		Params.mirostat = Miro;
		return Params;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType )
class EXAMPLE_API ULlamaComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULlamaComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = llama)
	FString ModelName = "qwen27-7b.q2.gguf";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = llama)
	UUserInterface* UserInterface;

	UFUNCTION(BlueprintCallable, Category = llama)
	TArray<FString> GetModelPaths();

	UFUNCTION(BlueprintCallable, Category = llama)
	void DownloadGGUFFile(const FString& Url, const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = llama)
	const bool IsInitialized();

	UFUNCTION(BlueprintCallable, Category = llama)
	void InitSampler(FSamplerParams InParams);

	UFUNCTION(BlueprintCallable, Category = llama)
	void InitGenerating(const FString& InInstructions, const FString& InText);
	
	UFUNCTION(BlueprintCallable, Category = llama)
	void StartGenerating();
	
	UFUNCTION(BlueprintCallable, Category = llama)
	void PauseGenerating();
	
	UFUNCTION(BlueprintCallable, Category = llama)
	void StopGenerating();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = llama)
	float RepeatPenalty = 1.1; // Bestraft Wiederholungen

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = llama)
	int n_len = 512;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = llama)
	int batchSize = 512;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = llama)
	int maxSeq = 0;

protected:

	void UnloadModel();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:

	llama_batch Batch;
	int CurrentIndex;
	FString ResponseText;

	llama_sampler* LlamaSampler;
	common_sampler* LlamaCommonSampler;

	bool IsGenerating = false;
	bool IsPaused = false;
	bool IsStopped = false;
	bool IsCancelled = false;

	// Helper functions
	void PrintString(const FString InString) { UKismetSystemLibrary::PrintString(GetWorld(), InString); }
	const bool is_valid_utf8(const char* string);
	const FString FilterPrintable(const FString& In);
	const FString RolePrompt(const FString& InInstructions, const FString& InText);

	int64 ContentLength;
	bool CancelDownload = false;
};
