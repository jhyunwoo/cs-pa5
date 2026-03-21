/************************************************************
 * CAS2107 – Programming Assignment 5
 * Unix File Diff and Patch Utility
 ************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// 문자열 길이를 계산하는 함수
size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++; // 널 종료 문자가 나올 때까지 길이 증가
    return len;
}

// 문자열을 비교하는 함수
int my_strcmp(const char *s1, const char *s2) {
    // 두 문자열의 문자가 같고 널 문자가 아닐 때까지 반복
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    // 차이가 발생한 지점의 아스키 코드 값 차이를 반환
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 문자열 n바이트를 비교하는 함수
int my_strncmp(const char *s1, const char *s2, size_t n) {
    // n이 0보다 크고 문자가 같고 널 문자가 아닐 때까지 반복
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0; // n바이트만큼 모두 같으면 0 반환
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 문자열을 정수로 변환하는 함수
long my_atoi(const char *s) {
    long res = 0;
    int sign = 1;
    if (*s == '-') { // 음수 처리
        sign = -1;
        s++;
    }
    // 숫자일 경우 자릿수를 높이며 값 누적
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

// 정수를 문자열로 변환하는 함수
char* my_itoa(long value, char *buf, int size) {
    int i = size - 1;
    buf[i] = '\0'; // 버퍼 끝에 널 문자 추가
    if (value == 0) {
        buf[--i] = '0';
        return &buf[i];
    }
    int negative = 0;
    if (value < 0) { // 음수 처리
        negative = 1;
        value = -value;
    }
    // 끝에서부터 역순으로 숫자 문자를 버퍼에 채움
    while (value > 0 && i > 0) {
        buf[--i] = (value % 10) + '0';
        value /= 10;
    }
    if (negative && i > 0) {
        buf[--i] = '-';
    }
    return &buf[i]; // 변환된 문자열의 시작 포인터 반환
}

// 쓰기 동작 중 시그널 등으로 인해 일부만 쓰이는 경우를 처리하는 함수
void write_all(int fd, const char *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        // 남은 바이트만큼 쓰기 시도
        ssize_t ret = write(fd, buf + total, count - total);
        if (ret < 0) {
            if (errno == EINTR) continue; // 인터럽트에 의한 실패면 재시도
            return; // 에러 발생 시 중단
        }
        total += ret; // 쓴 만큼 누적함
    }
}

// 파일 디스크립터에 문자열을 출력하는 함수
void print_str(int fd, const char *s) {
    write_all(fd, s, my_strlen(s));
}

// 파일 디스크립터에 정수를 출력하는 함수
void print_long(int fd, long val) {
    char buf[32];
    char *s = my_itoa(val, buf, 32);
    print_str(fd, s);
}

// diff 함수
void run_diff(const char *orig_path, const char *mod_path, const char *patch_path) {
    // 원본 및 수정본 파일을 열어 저장
    int fd_orig = open(orig_path, O_RDONLY);
    if (fd_orig < 0) return;
    int fd_mod = open(mod_path, O_RDONLY);
    if (fd_mod < 0) {
        close(fd_orig);
        return;
    }

    // 파일 크기 정보를 가져옴
    struct stat st_orig, st_mod;
    if (fstat(fd_orig, &st_orig) < 0 || fstat(fd_mod, &st_mod) < 0) {
        close(fd_orig);
        close(fd_mod);
        return;
    }

    off_t size_orig = st_orig.st_size;
    off_t size_mod = st_mod.st_size;
    off_t loop_limit = size_mod; // 수정된 파일 크기만큼 비교 루프 진행

    // 패치 파일 헤더에 'NUM CHANGES'를 적어야 하기 때문에 먼저 훑어서 변경 덩어리가 몇 개인지 셈
    int num_changes = 0;
    off_t pos = 0;
    int in_change = 0; // 현재 변경 구간 내부에 있는지 표시하는 플래그
    
    char b_o, b_m;
    ssize_t ret_o, ret_m;

    while (pos < loop_limit) {
        int has_o = (pos < size_orig); // 원본 파일 범위 내인지 확인
        
        // 원본 파일에서 1바이트 읽기
        if (has_o) {
            ret_o = read(fd_orig, &b_o, 1);
            if (ret_o != 1) b_o = 0; 
        } else { // 범위 밖이면 0 처리
            b_o = 0; 
        }

        // 수정된 파일에서 1바이트 읽기
        ret_m = read(fd_mod, &b_m, 1); 
        if (ret_m != 1) b_m = 0;

        // 차이 감지
        int diff = 0;
        if (has_o) {
            if (b_o != b_m) diff = 1; // 내용이 다르면 diff
        } else {
            diff = 1; // 원본 범위를 벗어났는데 수정본 데이터가 있다면 diff
        }

        // 변경 구간 상태 관리
        if (in_change) {
            if (!diff) {
                // 변경 구간에 있었는데 내용이 같아지면 변경 구간 종료
                in_change = 0;
            }
        } else {
            if (diff) {
                // 변경 구간이 아니었는데 차이 발생하면 새로운 변경 시작
                in_change = 1;
                num_changes++;
            }
        }
        pos++;
    }
    
    // 패치 파일 생성 및 헤더 작성
    int fd_patch = open(patch_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_patch < 0) {
        close(fd_orig);
        close(fd_mod);
        return;
    }

    print_str(fd_patch, "ORIGINAL_SIZE: ");
    print_long(fd_patch, (long)size_orig);
    print_str(fd_patch, "\nMODIFIED_SIZE: ");
    print_long(fd_patch, (long)size_mod);
    print_str(fd_patch, "\nNUM_CHANGES: ");
    print_long(fd_patch, (long)num_changes);
    print_str(fd_patch, "\n");

    //파일을 다시 처음으로 돌리고 실제 변경 내용을 기록함.
    lseek(fd_orig, 0, SEEK_SET);
    lseek(fd_mod, 0, SEEK_SET);

    pos = 0;
    in_change = 0;
    off_t change_start = 0;
    int change_counter = 0;

    while (pos < loop_limit) {
        int has_o = (pos < size_orig);
        
        // 바이트 읽기
        if (has_o) {
            ret_o = read(fd_orig, &b_o, 1);
            if (ret_o != 1) b_o = 0;
        } else {
            b_o = 0;
        }
        ret_m = read(fd_mod, &b_m, 1);
        if (ret_m != 1) b_m = 0;

        int diff = 0;
        if (has_o) {
            if (b_o != b_m) diff = 1;
        } else {
            diff = 1;
        }

        // 변경 구간 기록
        if (in_change) {
            if (!diff) {
                // 변경 구간 종료
                in_change = 0;
                off_t change_end = pos - 1;
                
                change_counter++;
                // 패치 파일 포맷에 맞춰 메타데이터 기록
                print_str(fd_patch, "---\nCHANGE #");
                print_long(fd_patch, change_counter);
                print_str(fd_patch, ":\nOFFSET: ");
                print_long(fd_patch, (long)change_start);
                print_str(fd_patch, "\nLENGTH: ");
                print_long(fd_patch, (long)(change_end - change_start + 1));
                print_str(fd_patch, "\nDATA: ");
                
                // 실제 데이터를 기록하기 위해 수정된 파일 포인터를 잠시 저장 후 이동
                off_t current_mod_pos = lseek(fd_mod, 0, SEEK_CUR);
                lseek(fd_mod, change_start, SEEK_SET);
                
                // 변경된 범위만큼 읽어서 패치 파일에 씀
                for (off_t k = change_start; k <= change_end; k++) {
                   char c;
                   if (read(fd_mod, &c, 1) == 1) {
                       if (c == '\n') print_str(fd_patch, "\\n"); // 개행 문자는 리터럴 "\n"으로 변환
                       else {
                           char tmp[1]; tmp[0]=c;
                           write_all(fd_patch, tmp, 1);
                       }
                   }
                }
                
                // 파일 포인터 원상 복구
                lseek(fd_mod, current_mod_pos, SEEK_SET);
                print_str(fd_patch, "\n");
            }
        } else {
            if (diff) {
                // 새로운 변경 시작 지점 기록
                in_change = 1;
                change_start = pos;
            }
        }
        pos++;
    }
    
    // 파일 끝까지 변경 구간이 이어진 경우 처리
    if (in_change) {
        change_counter++;
        off_t change_end = pos - 1;
        print_str(fd_patch, "---\nCHANGE #");
        print_long(fd_patch, change_counter);
        print_str(fd_patch, ":\nOFFSET: ");
        print_long(fd_patch, (long)change_start);
        print_str(fd_patch, "\nLENGTH: ");
        print_long(fd_patch, (long)(change_end - change_start + 1));
        print_str(fd_patch, "\nDATA: ");
        
        off_t current_mod_pos = lseek(fd_mod, 0, SEEK_CUR);
        lseek(fd_mod, change_start, SEEK_SET);
        for (off_t k = change_start; k <= change_end; k++) {
            char c;
            if (read(fd_mod, &c, 1) == 1) {
                if (c == '\n') print_str(fd_patch, "\\n");
                else {
                    char tmp[1]; tmp[0]=c;
                    write_all(fd_patch, tmp, 1);
                }
            }
        }
        lseek(fd_mod, current_mod_pos, SEEK_SET);
        print_str(fd_patch, "\n");
    }

    close(fd_orig);
    close(fd_mod);
    close(fd_patch);
}

// 한 줄 읽기 함수
int read_line(int fd, char *buf, int max_len) {
    int i = 0;
    char c;
    while (i < max_len - 1) {
        if (read(fd, &c, 1) != 1) break; // 읽기 실패 혹은 EOF
        buf[i++] = c;
        if (c == '\n') break; // 개행 만나면 중단
    }
    buf[i] = '\0'; // 문자열 종료
    return i;
}

// 문자열 접두사를 확인하는 함수
int starts_with(const char *str, const char *prefix) {
    return my_strncmp(str, prefix, my_strlen(prefix)) == 0;
}

// 문자열에서 정수를 파싱하는 함수
long parse_value(const char *line) {
    const char *p = line;
    while (*p && *p != ':') p++; // 콜론 찾기
    if (*p != ':') return 0;
    p++; 
    while (*p == ' ') p++; // 공백 건너뛰기
    return my_atoi(p); // 정수 변환
}

// 패치 적용 함수
void run_patch(const char *orig_path, const char *patch_path, const char *out_path) {
    // 파일 열기 진행
    int fd_orig = open(orig_path, O_RDONLY);
    if (fd_orig < 0) return;
    int fd_patch = open(patch_path, O_RDONLY);
    if (fd_patch < 0) {
       close(fd_orig);
       return;
    }
    // 출력 파일 생성
    int fd_out = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        close(fd_orig);
        close(fd_patch);
        return;
    }

    // 원본 파일 내용을 출력 파일로 복사
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(fd_orig, buffer, sizeof(buffer))) > 0) {
        write_all(fd_out, buffer, bytes);
    }
    
    char line[1024];
    long mod_size = -1;
    
    // 패치 파일 헤더 파싱
    read_line(fd_patch, line, sizeof(line)); // ORIGINAL SIZE 읽기
    read_line(fd_patch, line, sizeof(line)); // MODIFIED SIZE 읽기
    if (starts_with(line, "MODIFIED_SIZE:")) {
        mod_size = parse_value(line); // 최종 목표 크기 저장
    }
    read_line(fd_patch, line, sizeof(line)); // NUM CHANGES 읽기
    
    // 변경 사항 적용
    while (read_line(fd_patch, line, sizeof(line)) > 0) {
        if (starts_with(line, "---")) { // 새로운 변경 블록 시작
            read_line(fd_patch, line, sizeof(line)); // CHANGE는 건너뜀
            
            read_line(fd_patch, line, sizeof(line)); //  OFFSET 읽기
            long offset = parse_value(line);
            
            read_line(fd_patch, line, sizeof(line)); // LENGTH 읽기
            
            //데이터가 매우 길 수 있으므로 read_line 대신 스트림 방식으로 읽음
            char c;
            int count = 0;
            // 6글자라고 가정하고 읽어냄
            while (count < 6 && read(fd_patch, &c, 1) == 1) {
                count++;
            }
            
            // 출력 파일의 변경 위치로 이동
            lseek(fd_out, offset, SEEK_SET);
            
            // 데이터 내용을 개행문자가 나올 때까지 읽어서 씀
            while (read(fd_patch, &c, 1) == 1) {
                if (c == '\n') break; // DATA 라인의 끝
                
                // 이스케이프 문자 처리
                if (c == '\\') {
                    char next_c;
                    if (read(fd_patch, &next_c, 1) == 1) {
                        if (next_c == 'n') {
                            char nl = '\n';
                            write_all(fd_out, &nl, 1);
                        } else {
                            // 일반 역슬래시인 경우 그대로 씀
                            write_all(fd_out, &c, 1);
                            write_all(fd_out, &next_c, 1);
                        }
                    }
                } else {
                    write_all(fd_out, &c, 1);
                }
            }
        }
    }
    
    // 파일 크기 조정
    // 원본보다 수정본이 작아진 경우 뒷부분을 잘라내야 함
    if (mod_size >= 0) {
        if (ftruncate(fd_out, mod_size) < 0) {}
    }

    close(fd_orig);
    close(fd_patch);
    close(fd_out);
}

// 두 파일이 완전히 동일한지 검증하는 함수
void run_verify(const char *file1, const char *file2) {
    int fd1 = open(file1, O_RDONLY);
    int fd2 = open(file2, O_RDONLY);
    if (fd1 < 0 || fd2 < 0) {
        if (fd1 >= 0) close(fd1);
        if (fd2 >= 0) close(fd2);
        return;
    }
    
    // 파일 크기 비교
    struct stat st1, st2;
    fstat(fd1, &st1);
    fstat(fd2, &st2);
    
    if (st1.st_size != st2.st_size) { // 다를 경우 처리
        print_str(STDOUT_FILENO, "Files are different\n");
        close(fd1);
        close(fd2);
        return;
    }
    
    // 바이트 단위 내용 비교
    char b1[4096], b2[4096];
    ssize_t n1, n2;
    int diff = 0;
    
    // 블록 단위로 읽어서 메모리 비교
    while ((n1 = read(fd1, b1, sizeof(b1))) > 0) {
        n2 = read(fd2, b2, sizeof(b2)); 
        if (n1 != n2) { diff = 1; break; } // 읽은 길이가 다르면 다름
        if (my_strncmp(b1, b2, n1) != 0) { diff = 1; break; } // 내용이 다르면 다름
    }
    
    if (diff) {
        print_str(STDOUT_FILENO, "Files are different\n");
    } else {
        print_str(STDOUT_FILENO, "Files are identical\n");
    }
    
    close(fd1);
    close(fd2);
}

// 프로그램 실행
int main(int argc, char *argv[]) {

    if (argc < 2) {
        write_all(STDERR_FILENO, "Invalid arguments\n", 18);
        return 1;
    }

    // ./patcher diff <orig> <mod> <patch>
    if (argc == 5 && !my_strcmp(argv[1], "diff")) {
        run_diff(argv[2], argv[3], argv[4]);
        return 0;
    }

    // ./patcher patch <orig> <patch> <out>
    if (argc == 5 && !my_strcmp(argv[1], "patch")) {
        run_patch(argv[2], argv[3], argv[4]);
        return 0;
    }

    // ./patcher verify <file1> <file2>
    if (argc == 4 && !my_strcmp(argv[1], "verify")) {
        run_verify(argv[2], argv[3]);
        return 0;
    }

    write_all(STDERR_FILENO, "Invalid arguments\n", 18);
    return 1;
}
