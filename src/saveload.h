#ifndef SAVELOAD_H
#define SAVELOAD_H

/** @defgroup saveload Save and Load
 *  @brief Functions to use the save and load screen.
 *  @{
 */

/** @def OSL_SAVELOAD_CANCEL
 *  @brief Indicates that the save/load operation was canceled.
 */
#define OSL_SAVELOAD_CANCEL 1

/** @def OSL_SAVELOAD_OK
 *  @brief Indicates that the save/load operation was successful.
 */
#define OSL_SAVELOAD_OK 0

/** @def OSL_DIALOG_SAVE
 *  @brief Indicates a save dialog type.
 */
#define OSL_DIALOG_SAVE 1

/** @def OSL_DIALOG_LOAD
 *  @brief Indicates a load dialog type.
 */
#define OSL_DIALOG_LOAD 2

/** @def OSL_DIALOG_DELETE
 *  @brief Indicates a delete dialog type.
 */
#define OSL_DIALOG_DELETE 3

/** @struct oslSaveLoad
 *  @brief Structure containing data to save/load.
 *  @details This structure holds all the necessary data to perform save, load, or delete operations.
 *  
 *  @param gameTitle
 *      Title of the game.
 *  @param gameID
 *      Game ID of the game.
 *  @param saveName
 *      Name of the save.
 *  @param savedataTitle
 *      ParamSfo SaveData Title.
 *  @param detail
 *      ParamSfo SaveData Detail.
 *  @param nameList
 *      List of names for save data.
 *  @param pic1
 *      Pic1 data.
 *  @param size_pic1
 *      Size of pic1.
 *  @param icon0
 *      Icon0 data.
 *  @param size_icon0
 *      Size of icon0.
 *  @param dialogType
 *      Type of save/load dialog. Example: 0 for multi-list, 1 for single data, 2 for automatic.
 *  @param data
 *      Pointer to data to be saved/loaded.
 *  @param dataSize
 *      Size of the data to be saved/loaded.
 */
struct oslSaveLoad {
    char gameTitle[0x80];
    char gameID[13];
    char saveName[20];
    char savedataTitle[0x80];
    char detail[0x400];
    char (*nameList)[20];
    unsigned char *pic1;
    unsigned int size_pic1;
    unsigned char *icon0;
    unsigned int size_icon0;
    int dialogType;
    void *data;
    int dataSize;
};

/** 
 * @brief Initializes the save dialog.
 * @param saveData Pointer to a oslSaveLoad structure containing data to be saved.
 */
void oslInitSaveDialog(struct oslSaveLoad *saveData);

/** 
 * @brief Initializes the multi-list save dialog.
 * @param saveData Pointer to a oslSaveLoad structure containing data to be saved.
 */
void oslInitMultiSaveDialog(struct oslSaveLoad *saveData);

/** 
 * @brief Initializes the single save dialog.
 * @param saveData Pointer to a oslSaveLoad structure containing data to be saved.
 */
void oslInitSingleSaveDialog(struct oslSaveLoad *saveData);

/** 
 * @brief Initializes the automatic save dialog.
 * @param saveData Pointer to a oslSaveLoad structure containing data to be saved.
 */
void oslInitAutoSaveDialog(struct oslSaveLoad *saveData);

/** 
 * @brief Initializes the load dialog.
 * @param loadData Pointer to a oslSaveLoad structure where data will be loaded.
 */
void oslInitLoadDialog(struct oslSaveLoad *loadData);

/** 
 * @brief Initializes the multi-list load dialog.
 * @param loadData Pointer to a oslSaveLoad structure where data will be loaded.
 */
void oslInitMultiLoadDialog(struct oslSaveLoad *loadData);

/** 
 * @brief Initializes the single load dialog.
 * @param loadData Pointer to a oslSaveLoad structure where data will be loaded.
 */
void oslInitSingleLoadDialog(struct oslSaveLoad *loadData);

/** 
 * @brief Initializes the automatic load dialog.
 * @param loadData Pointer to a oslSaveLoad structure where data will be loaded.
 */
void oslInitAutoLoadDialog(struct oslSaveLoad *loadData);

/** 
 * @brief Initializes the delete dialog.
 * @param deleteData Pointer to a oslSaveLoad structure where data will be deleted.
 */
void oslInitDeleteDialog(struct oslSaveLoad *deleteData);

/** 
 * @brief Draws the save/load dialog.
 */
void oslDrawSaveLoad(void);

/** 
 * @brief Returns the current dialog status.
 * @return The status of the current save/load dialog.
 */
int oslGetLoadSaveStatus(void);

/** 
 * @brief Returns the current dialog type.
 * @return The type of the current save/load dialog.
 */
int oslGetSaveLoadType(void);

/** 
 * @brief Gets the dialog's result.
 * @return The result of the save/load operation.
 */
int oslSaveLoadGetResult(void);

/** 
 * @brief Ends the current dialog.
 */
void oslEndSaveLoadDialog(void);

/** @} */ // end of saveload

#endif // SAVELOAD_H
