#include <windows.h>
#include <stdio.h>
#pragma comment(lib,"user32.lib")
DWORD addrFunction,addrIAFuntion;
char *hacked="I\'m hacked!";
char *title="hello";

DWORD GetIAFromImportTable(DWORD dwBase,LPCSTR lpszFuncName){
	PIMAGE_DOS_HEADER 			pDosHeader;
  PIMAGE_NT_HEADERS 			pNtHeaders;
  PIMAGE_FILE_HEADER 			pFileHeader;
  PIMAGE_OPTIONAL_HEADER32 	pOptHeader;
  DWORD dwRVAImpTBL;
  DWORD dwSizeOfImpTBL;
  PIMAGE_IMPORT_DESCRIPTOR pImpTBL,p;
  PIMAGE_IMPORT_BY_NAME pOrdName;
  DWORD dwRVAName;
  PIMAGE_THUNK_DATA thunkTargetFunc;
  WORD hint;
  
  
  DWORD dwIA=0;
  pDosHeader = (PIMAGE_DOS_HEADER)dwBase;
  pNtHeaders = (PIMAGE_NT_HEADERS)(dwBase + pDosHeader->e_lfanew);
  pOptHeader = &(pNtHeaders->OptionalHeader);
  dwRVAImpTBL = pOptHeader->DataDirectory[1].VirtualAddress;
  dwSizeOfImpTBL = pOptHeader->DataDirectory[1].Size;
  pImpTBL = (PIMAGE_IMPORT_DESCRIPTOR)(dwBase + dwRVAImpTBL);
  for (p = pImpTBL; (DWORD)p < ((DWORD)pImpTBL + dwSizeOfImpTBL); p++){
    	//dwRVAName=p->Name;//dwRVAName是该描述符的name属性，为RVA
    	thunkTargetFunc=(PIMAGE_THUNK_DATA)GetIAFromImpDesc(dwBase,lpszFuncName,p);
    	if(!thunkTargetFunc){
    		continue;
    		}
    		else{
    			printf("0x%08x ==> 0x%08x\n",&(thunkTargetFunc->u1.Function),thunkTargetFunc->u1.Function);
    			addrIAFuntion=thunkTargetFunc->u1.Function;//Function的值，即入口地址
    			addrFunction=&(thunkTargetFunc->u1.Function);//IAT里面function所在的地    			
    			}	
    			
    }
    
    return 0;
	}
	
	
	
DWORD GetIAFromImpDesc(DWORD dwBase, LPCSTR lpszName,PIMAGE_IMPORT_DESCRIPTOR pImpDesc) 
{
    PIMAGE_THUNK_DATA pthunk, pthunk2;
    PIMAGE_IMPORT_BY_NAME pOrdinalName;
    if (pImpDesc->Name == 0) return 0;
    pthunk = (PIMAGE_THUNK_DATA) (dwBase + pImpDesc->OriginalFirstThunk);
    pthunk2 = (PIMAGE_THUNK_DATA) (dwBase + pImpDesc->FirstThunk);
    for (; pthunk->u1.Function != 0; pthunk++, pthunk2++) {
        if (pthunk->u1.Ordinal & 0x80000000) continue;//如果是序号值就跳过
        pOrdinalName = (PIMAGE_IMPORT_BY_NAME) (dwBase + pthunk->u1.AddressOfData);//AddressOfData指向imageImportByName结构的RVA
        if (CompStr((LPSTR)lpszName, (LPSTR)&pOrdinalName->Name)) //Name 在结构里面直接就是字符串，我们要获取其首地址
            return (DWORD)pthunk2;
    }
    return 0;
}
BOOL CompStr(LPSTR s1, LPSTR s2)
{
    PCHAR p, q;
    for (p = s1, q = s2; (*p != 0) && (*q != 0); p++, q++) {
        if (*p != *q) return FALSE;
    }
    return TRUE;
}
__declspec(naked) MyMessageBoxA()
{
     
     __asm{
     	push ebp
     	mov ebp,esp
     	push 4
     	push title
     	push hacked
     	push 0
     	mov eax,dword ptr [addrIAFuntion]
     	call eax
     	add esp,32
     	mov esp,ebp
     	pop ebp
     	ret 
     	
     	}
}
// test.c
void main()
{
		DWORD dwBase;
		DWORD addrMMBA;
		int old;
		int num;
		dwBase = (DWORD) GetModuleHandleA(NULL);
		
		//printf("%08x\n",MessageBox);
    GetIAFromImportTable(dwBase,"MessageBoxA");
    __asm{
    	lea edx,MyMessageBoxA
    	mov dword ptr [addrMMBA],edx
    	}
    	printf("MyMessageBoxA Addr:0x%08x\n",addrMMBA);
    	
    if(!VirtualProtect(addrFunction, 4, PAGE_EXECUTE_READWRITE, &old)){
		//修改代码段为可写
			printf("[E]Change to be Writable Failed\n");
		}else 
		{
			printf("[I]Change to be Writable Success\n");
		}
		if(!WriteProcessMemory( INVALID_HANDLE_VALUE,addrFunction, &addrMMBA,  4, &num)){
			printf("[E]Write Addr Failed\n");
			}
		else{
			printf("[I]Write Addr Success\n");
			}
			//printf("addrIAFuntion Addr:0x%08x\n",addrFunction);
			printf("MessageBoxA Addr：0x%08x\n",MessageBoxA);
    MessageBoxA(NULL, "hello", "msg", MB_OK);
} 



