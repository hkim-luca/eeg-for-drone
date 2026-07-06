#pragma once

#include "CoreMinimal.h"
#include "DroneSimCharacter.generated.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"

class USkeletalMesh;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ADroneSimCharacter : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent *CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent *FollowCamera;

  protected:
    /** Jump Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *LookAction;

    /** Mouse Look Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *MouseLookAction;

  public:
    /** Constructor */
    ADroneSimCharacter();

  protected:
    /** Initialize input action bindings */
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

  protected:
    /** Called for movement input */
    void Move(const FInputActionValue &Value);

    /** Called for looking input */
    void Look(const FInputActionValue &Value);

  public:
    /** Handles move inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoMove(float Right, float Forward);

    /** Handles look inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoLook(float Yaw, float Pitch);

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpStart();

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpEnd();

  public:
    /** Swaps the body to the given skeletal mesh asset (the airframe model declared
     *  in Content/Drones/DronePresets.json); called by the drone physics when a run
     *  starts and when the preset changes live. An empty path keeps the Blueprint's
     *  own body; a path that fails to load keeps the body and logs once. */
    void ApplyAirframeMesh(const FString &MeshPath);

  protected:
    /** Mesh paths already reported as unloadable, to log each of them only once */
    TSet<FString> WarnedMissingAirframes;

  public:
    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent *GetCameraBoom() const
    {
        return CameraBoom;
    }

    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent *GetFollowCamera() const
    {
        return FollowCamera;
    }
};
